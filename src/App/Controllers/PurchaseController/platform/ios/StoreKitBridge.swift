import Foundation
import StoreKit

private struct TipProductPayload: Encodable {
  let id: String
  let title: String
  let displayPrice: String
}

@MainActor
private final class PastViewerStore {
  static let shared = PastViewerStore()

  private var products: [String: Product] = [:]
  private var transactionListener: Task<Void, Never>?
  private var verifiedTransactionCallback: TransactionCallback?
  private var purchaseEndedCallback: PurchaseEndedCallback?
  private var reportedFailureTransactionIds: Set<UInt64> = []

  func loadProducts(productIds: [String]) async throws -> [TipProductPayload] {
    startTransactionListener()

    let loadedProducts = try await Product.products(for: productIds)
    products = Dictionary(uniqueKeysWithValues: loadedProducts.map { ($0.id, $0) })

    return productIds.compactMap { productId in
      guard let product = products[productId] else {
        return nil
      }

      return TipProductPayload(
        id: product.id,
        title: product.displayName,
        displayPrice: product.displayPrice)
    }
  }

  func purchase(productId: String, nonConsumable: Bool) async throws -> Int32 {
    startTransactionListener()

    let product: Product
    if let cached = products[productId] {
      product = cached
    } else if let loaded = try await Product.products(for: [productId]).first {
      products[productId] = loaded
      product = loaded
    } else {
      throw nonConsumable ? StoreError.tourUnavailable : StoreError.tipUnavailable
    }

    if nonConsumable && product.type != .nonConsumable {
      throw StoreError.notNonConsumable
    }

    switch try await product.purchase() {
    case .success(let verificationResult):
      switch verificationResult {
      case .verified(let transaction):
        await transaction.finish()
        return 0
      case .unverified(_, let error):
        throw error
      }

    case .userCancelled:
      return 1

    case .pending:
      return 2

    @unknown default:
      throw StoreError.unknownPurchaseResult
    }
  }

  func hasVerifiedEntitlement(productId: String) async throws -> Bool {
    startTransactionListener()
    await finishUnfinishedTransactions()

    for await result in Transaction.currentEntitlements {
      guard case .verified(let transaction) = result else {
        continue
      }
      if transaction.productID == productId && transaction.revocationDate == nil {
        return true
      }
    }
    return false
  }

  func setVerifiedTransactionCallback(_ callback: @escaping TransactionCallback) {
    verifiedTransactionCallback = callback
    startTransactionListener()
  }

  func setPurchaseEndedCallback(_ callback: @escaping PurchaseEndedCallback) {
    purchaseEndedCallback = callback
  }

  func purchasePendingState(productId: String) async throws -> Int32 {
    // Ask to Buy waiting on approval is not a verified unfinished transaction.
    // An empty Transaction.unfinished queue stays pending until
    // currentEntitlements contains this product. hasVerifiedEntitlement finishes
    // verified transactions before that scan.
    if try await hasVerifiedEntitlement(productId: productId) {
      return 0
    }
    return 1
  }

  private func startTransactionListener() {
    guard transactionListener == nil else { return }
    transactionListener = Task { @MainActor in
      await withTaskGroup(of: Void.self) { group in
        group.addTask { @MainActor in
          while !Task.isCancelled {
            do {
              try await PastViewerStore.shared.consumeTransactionUpdates()
            } catch {
              // Transaction.updates stopped. Subscribe again so a later transaction is finished.
            }
            if Task.isCancelled {
              break
            }
            // This pause is not the only chance to finish a transaction. The
            // unfinished scan keeps running and finishes one that arrives here.
            try? await Task.sleep(nanoseconds: 1_000_000_000)
          }
        }
        group.addTask { @MainActor in
          while !Task.isCancelled {
            await PastViewerStore.shared.finishUnfinishedTransactions()
            if Task.isCancelled {
              break
            }
            try? await Task.sleep(nanoseconds: 1_000_000_000)
          }
        }
      }
    }
  }

  func finishUnfinishedTransactions() async {
    do {
      for await verificationResult in Transaction.unfinished {
        await finishVerifiedTransaction(verificationResult)
      }
    } catch {
      // Try again on the next pass. currentEntitlements is still checked separately.
    }
  }

  private func consumeTransactionUpdates() async throws {
    for await verificationResult in Transaction.updates {
      await finishVerifiedTransaction(verificationResult)
    }
  }

  private func finishVerifiedTransaction(_ verificationResult: VerificationResult<Transaction>) async {
    switch verificationResult {
    case .verified(let transaction):
      let productId = transaction.productID
      await transaction.finish()

      if transaction.revocationDate != nil {
        // Finished above. A refund is not an entitlement and not a purchase
        // failure, so the pending tour message stays until a verified one exists.
        return
      }

      notifyVerifiedTransaction(productId)

    case .unverified(let transaction, let error):
      // Leave the transaction unfinished. StoreKit reported a real failure, so
      // the pending tour message is cleared and the error is shown once.
      reportPurchaseFailure(transaction.id, transaction.productID, error.localizedDescription)
    }
  }

  private func reportPurchaseFailure(_ transactionId: UInt64, _ productId: String, _ errorMessage: String) {
    let firstReport = reportedFailureTransactionIds.insert(transactionId).inserted
    guard firstReport else { return }
    notifyPurchaseEnded(productId, failed: true, errorMessage: errorMessage)
  }

  private func notifyVerifiedTransaction(_ productId: String) {
    guard let verifiedTransactionCallback else { return }
    productId.withCString { productId in
      verifiedTransactionCallback(productId)
    }
  }

  private func notifyPurchaseEnded(_ productId: String, failed: Bool, errorMessage: String?) {
    guard let purchaseEndedCallback else { return }
    let failedFlag: Int32 = failed ? 1 : 0
    productId.withCString { productId in
      if let errorMessage {
        errorMessage.withCString { errorMessage in
          purchaseEndedCallback(productId, failedFlag, errorMessage)
        }
      } else {
        purchaseEndedCallback(productId, failedFlag, nil)
      }
    }
  }

  private enum StoreError: LocalizedError {
    case tipUnavailable
    case tourUnavailable
    case notNonConsumable
    case unknownPurchaseResult

    var errorDescription: String? {
      switch self {
      case .tipUnavailable:
        return "The selected tip is unavailable."
      case .tourUnavailable:
        return "This tour is unavailable in the App Store."
      case .notNonConsumable:
        return "This tour is not available as a one-time purchase."
      case .unknownPurchaseResult:
        return "The App Store returned an unknown purchase result."
      }
    }
  }
}

typealias ProductsCallback =
  @convention(c) (
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?
  ) -> Void

typealias PurchaseCallback =
  @convention(c) (
    Int32,
    UnsafePointer<CChar>?
  ) -> Void

typealias OwnershipCallback =
  @convention(c) (
    UInt64,
    Int32
  ) -> Void

typealias TransactionCallback =
  @convention(c) (
    UnsafePointer<CChar>?
  ) -> Void

typealias PurchaseEndedCallback =
  @convention(c) (
    UnsafePointer<CChar>?,
    Int32,
    UnsafePointer<CChar>?
  ) -> Void

@_cdecl("PastViewerStoreKitLoadProducts")
func loadPastViewerStoreKitProducts(
  _ productIdsCsv: UnsafePointer<CChar>?,
  _ callback: @escaping ProductsCallback
) {
  guard let productIdsCsv else {
    "No App Store tip products were configured.".withCString { callback(nil, $0) }
    return
  }

  let productIds = String(cString: productIdsCsv)
    .split(separator: ",")
    .map(String.init)
    .filter { !$0.isEmpty }

  guard !productIds.isEmpty else {
    "No App Store tip products were configured.".withCString { callback(nil, $0) }
    return
  }

  Task { @MainActor in
    do {
      let products = try await PastViewerStore.shared.loadProducts(productIds: productIds)
      let data = try JSONEncoder().encode(products)
      guard let json = String(data: data, encoding: .utf8) else {
        throw EncodingError.invalidValue(
          products,
          EncodingError.Context(
            codingPath: [],
            debugDescription: "Could not encode StoreKit products."))
      }

      json.withCString { callback($0, nil) }
    } catch {
      error.localizedDescription.withCString { callback(nil, $0) }
    }
  }
}

@_cdecl("PastViewerStoreKitPurchase")
func purchasePastViewerStoreKitProduct(
  _ productId: UnsafePointer<CChar>?,
  _ nonConsumable: Int32,
  _ callback: @escaping PurchaseCallback
) {
  guard let productId else {
    "No App Store product was selected.".withCString { callback(3, $0) }
    return
  }

  let selectedProductId = String(cString: productId)
  let requireNonConsumable = nonConsumable != 0
  Task { @MainActor in
    do {
      callback(
        try await PastViewerStore.shared.purchase(
          productId: selectedProductId,
          nonConsumable: requireNonConsumable),
        nil)
    } catch {
      error.localizedDescription.withCString { callback(3, $0) }
    }
  }
}

@_cdecl("PastViewerStoreKitCheckOwned")
func checkPastViewerStoreKitOwned(
  _ productId: UnsafePointer<CChar>?,
  _ requestId: UInt64,
  _ callback: @escaping OwnershipCallback
) {
  guard let productId else {
    callback(requestId, 0)
    return
  }

  let selectedProductId = String(cString: productId)
  Task { @MainActor in
    do {
      let owned = try await PastViewerStore.shared.hasVerifiedEntitlement(productId: selectedProductId)
      callback(requestId, owned ? 1 : 0)
    } catch {
      callback(requestId, 0)
    }
  }
}

@_cdecl("PastViewerStoreKitQueryPending")
func queryPastViewerStoreKitPending(
  _ productId: UnsafePointer<CChar>?,
  _ requestId: UInt64,
  _ callback: @escaping OwnershipCallback
) {
  guard let productId else {
    callback(requestId, 0)
    return
  }

  let selectedProductId = String(cString: productId)
  Task { @MainActor in
    do {
      let state = try await PastViewerStore.shared.purchasePendingState(productId: selectedProductId)
      callback(requestId, state)
    } catch {
      callback(requestId, -1)
    }
  }
}

@_cdecl("PastViewerStoreKitStartPurchaseEndedListener")
func startPastViewerStoreKitPurchaseEndedListener(
  _ callback: @escaping PurchaseEndedCallback
) {
  Task { @MainActor in
    PastViewerStore.shared.setPurchaseEndedCallback(callback)
  }
}

@_cdecl("PastViewerStoreKitStartTransactionListener")
func startPastViewerStoreKitTransactionListener(
  _ callback: @escaping TransactionCallback
) {
  Task { @MainActor in
    PastViewerStore.shared.setVerifiedTransactionCallback(callback)
  }
}
