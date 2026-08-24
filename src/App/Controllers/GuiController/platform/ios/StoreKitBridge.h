#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (*PastViewerStoreKitProductsCallback)(const char * productsJson, const char * errorMessage);
	typedef void (*PastViewerStoreKitPurchaseCallback)(int result, const char * errorMessage);

	void PastViewerStoreKitLoadProducts(
		const char * productIdsCsv,
		PastViewerStoreKitProductsCallback callback);
	void PastViewerStoreKitPurchase(
		const char * productId,
		PastViewerStoreKitPurchaseCallback callback);

#ifdef __cplusplus
}
#endif
