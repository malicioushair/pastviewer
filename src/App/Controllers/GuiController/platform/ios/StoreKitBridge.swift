import Foundation
import StoreKit

private struct TipProductPayload: Encodable {
  let id: String
  let title: String
  let displayPrice: String
}

@MainActor
private final class PastViewerTipStore {
  static let shared = PastViewerTipStore()

  private var products: [String: Product] = [:]
  private var transactionListener: Task<Void, Never>?

  func loadProducts(productIds: [String]) async throws -> [TipProductPayload] {
    startTransactionListener(productIds: Set(productIds))

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

  func purchase(productId: String) async throws -> Int32 {
    guard let product = products[productId] else {
      throw StoreError.productUnavailable
    }

    switch try await product.purchase() {
    case .success(let verificationResult):
      guard case .verified(let transaction) = verificationResult else {
        throw StoreError.transactionUnverified
      }

      await transaction.finish()
      return 0

    case .userCancelled:
      return 1

    case .pending:
      return 2

    @unknown default:
      throw StoreError.unknownPurchaseResult
    }
  }

  private func startTransactionListener(productIds: Set<String>) {
    guard transactionListener == nil else {
      return
    }

    transactionListener = Task {
      for await verificationResult in Transaction.updates {
        guard case .verified(let transaction) = verificationResult,
          productIds.contains(transaction.productID)
        else {
          continue
        }

        await transaction.finish()
      }
    }

    Task {
      for await verificationResult in Transaction.unfinished {
        guard case .verified(let transaction) = verificationResult,
          productIds.contains(transaction.productID)
        else {
          continue
        }

        await transaction.finish()
      }
    }
  }

  private enum StoreError: LocalizedError {
    case productUnavailable
    case transactionUnverified
    case unknownPurchaseResult

    var errorDescription: String? {
      switch self {
      case .productUnavailable:
        return "The selected tip is unavailable."
      case .transactionUnverified:
        return "The App Store could not verify this purchase."
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
      let products = try await PastViewerTipStore.shared.loadProducts(productIds: productIds)
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
  _ callback: @escaping PurchaseCallback
) {
  guard let productId else {
    "No App Store tip product was selected.".withCString { callback(3, $0) }
    return
  }

  let selectedProductId = String(cString: productId)
  Task { @MainActor in
    do {
      callback(try await PastViewerTipStore.shared.purchase(productId: selectedProductId), nil)
    } catch {
      error.localizedDescription.withCString { callback(3, $0) }
    }
  }
}
