#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (*PastViewerStoreKitProductsCallback)(const char * productsJson, const char * errorMessage);
	typedef void (*PastViewerStoreKitPurchaseCallback)(int result, const char * errorMessage);
	typedef void (*PastViewerStoreKitOwnershipCallback)(uint64_t requestId, int owned);
	typedef void (*PastViewerStoreKitTransactionCallback)(const char * productId);
	typedef void (*PastViewerStoreKitPurchaseEndedCallback)(const char * productId, int failed, const char * errorMessage);

	void PastViewerStoreKitLoadProducts(
		const char * productIdsCsv,
		PastViewerStoreKitProductsCallback callback);
	void PastViewerStoreKitPurchase(
		const char * productId,
		int nonConsumable,
		PastViewerStoreKitPurchaseCallback callback);
	void PastViewerStoreKitCheckOwned(
		const char * productId,
		uint64_t requestId,
		PastViewerStoreKitOwnershipCallback callback);
	void PastViewerStoreKitQueryPending(
		const char * productId,
		uint64_t requestId,
		PastViewerStoreKitOwnershipCallback callback);
	void PastViewerStoreKitStartTransactionListener(PastViewerStoreKitTransactionCallback callback);
	void PastViewerStoreKitStartPurchaseEndedListener(PastViewerStoreKitPurchaseEndedCallback callback);

#ifdef __cplusplus
}
#endif
