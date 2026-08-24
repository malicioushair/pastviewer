package org.qtproject.PastViewer;

import android.app.Activity;
import android.util.Log;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.ConsumeParams;
import com.android.billingclient.api.PendingPurchasesParams;
import com.android.billingclient.api.ProductDetails;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.QueryProductDetailsParams;
import com.android.billingclient.api.QueryProductDetailsResult;
import com.android.billingclient.api.QueryPurchasesParams;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public final class GooglePlayBillingHelper {
    private static final String TAG = "PastViewerBilling";

    enum PurchaseResult {
        Succeeded(0),
        Canceled(1),
        Pending(2),
        Failed(3);

        private final int code;
        private PurchaseResult(int code) { this.code = code; }
        int toInt() { return this.code; }
    }

    private interface FailureHandler {
        void failed(String message);
    }

    private static final class ProductOffer {
        final ProductDetails productDetails;
        final ProductDetails.OneTimePurchaseOfferDetails offerDetails;

        ProductOffer(
                ProductDetails productDetails,
                ProductDetails.OneTimePurchaseOfferDetails offerDetails) {
            this.productDetails = productDetails;
            this.offerDetails = offerDetails;
        }
    }

    private static BillingClient billingClient;
    private static boolean connecting;
    private static Runnable pendingReadyAction;
    private static FailureHandler pendingFailureHandler;
    private static final List<String> configuredProductIds = new ArrayList<>();
    private static final Map<String, ProductOffer> products = new HashMap<>();
    private static final Set<String> consumptionsInProgress = new HashSet<>();
    private static String activeProductId;
    private static boolean purchaseCallbackPending;

    private GooglePlayBillingHelper() {}

    private static native void productsLoaded(String productsJson, String errorMessage);
    private static native void purchaseFinished(int result, String errorMessage);

    public static void initialize(Activity activity, String productIdsCsv) {
        if (activity == null) {
            Log.e(TAG, "Cannot initialize billing without an Activity");
            return;
        }

        activity.runOnUiThread(() -> {
            configureProductIds(productIdsCsv);
            if (configuredProductIds.isEmpty()) {
                return;
            }
            runWhenReady(activity, null, message -> Log.e(TAG, message));
        });
    }

    public static void loadProducts(Activity activity, String productIdsCsv) {
        if (activity == null) {
            productsLoaded(null, "Android activity is unavailable.");
            return;
        }

        activity.runOnUiThread(() -> {
            configureProductIds(productIdsCsv);
            if (configuredProductIds.isEmpty()) {
                productsLoaded(null, "No Google Play tip products were configured.");
                return;
            }

            runWhenReady(
                    activity,
                    GooglePlayBillingHelper::queryProducts,
                    message -> productsLoaded(null, message));
        });
    }

    public static void purchaseProduct(Activity activity, String productId) {
        if (activity == null || productId == null || productId.isEmpty()) {
            purchaseFinished(PurchaseResult.Failed.toInt(), "No Google Play tip product was selected.");
            return;
        }

        activity.runOnUiThread(() -> {
            if (purchaseCallbackPending) {
                purchaseFinished(PurchaseResult.Failed.toInt(), "Another Google Play purchase is already in progress.");
                return;
            }

            ProductOffer product = products.get(productId);
            if (product == null) {
                purchaseFinished(PurchaseResult.Failed.toInt(), "The selected Google Play tip is unavailable.");
                return;
            }

            activeProductId = productId;
            purchaseCallbackPending = true;
            runWhenReady(
                    activity,
                    () -> launchBillingFlow(activity, product),
                    message -> finishPurchase(PurchaseResult.Failed.toInt(), message));
        });
    }

    private static void configureProductIds(String productIdsCsv) {
        LinkedHashSet<String> uniqueIds = new LinkedHashSet<>();
        if (productIdsCsv != null) {
            for (String id : productIdsCsv.split(",")) {
                String trimmedId = id.trim();
                if (!trimmedId.isEmpty()) {
                    uniqueIds.add(trimmedId);
                }
            }
        }

        if (!configuredProductIds.equals(new ArrayList<>(uniqueIds))) {
            products.clear();
        }
        configuredProductIds.clear();
        configuredProductIds.addAll(uniqueIds);
    }

    private static void runWhenReady(
            Activity activity,
            Runnable readyAction,
            FailureHandler failureHandler) {
        ensureBillingClient(activity);
        if (billingClient.isReady()) {
            if (readyAction != null) {
                readyAction.run();
            }
            return;
        }

        if (readyAction != null) {
            if (pendingReadyAction != null) {
                failureHandler.failed("Another Google Play Billing request is already waiting for a connection.");
                return;
            }
            pendingReadyAction = readyAction;
            pendingFailureHandler = failureHandler;
        }

        if (connecting) {
            return;
        }

        connecting = true;
        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(BillingResult billingResult) {
                connecting = false;
                if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                    failPendingRequest(billingMessage("Could not connect to Google Play Billing", billingResult));
                    return;
                }

                Runnable action = pendingReadyAction;
                pendingReadyAction = null;
                pendingFailureHandler = null;
                if (action != null) {
                    action.run();
                }
                queryOutstandingPurchases();
            }

            @Override
            public void onBillingServiceDisconnected() {
                connecting = false;
            }
        });
    }

    private static void ensureBillingClient(Activity activity) {
        if (billingClient != null) {
            return;
        }

        PendingPurchasesParams pendingPurchasesParams = PendingPurchasesParams.newBuilder()
                .enableOneTimeProducts()
                .build();
        billingClient = BillingClient.newBuilder(activity.getApplicationContext())
                .setListener(GooglePlayBillingHelper::onPurchasesUpdated)
                .enablePendingPurchases(pendingPurchasesParams)
                .enableAutoServiceReconnection()
                .build();
    }

    private static void failPendingRequest(String message) {
        FailureHandler failureHandler = pendingFailureHandler;
        pendingReadyAction = null;
        pendingFailureHandler = null;
        if (failureHandler != null) {
            failureHandler.failed(message);
        } else {
            Log.e(TAG, message);
        }
    }

    private static void queryProducts() {
        List<QueryProductDetailsParams.Product> queryProducts = new ArrayList<>();
        for (String productId : configuredProductIds) {
            queryProducts.add(QueryProductDetailsParams.Product.newBuilder()
                    .setProductId(productId)
                    .setProductType(BillingClient.ProductType.INAPP)
                    .build());
        }

        QueryProductDetailsParams params = QueryProductDetailsParams.newBuilder()
                .setProductList(queryProducts)
                .build();
        billingClient.queryProductDetailsAsync(params, GooglePlayBillingHelper::onProductDetailsResponse);
    }

    private static void onProductDetailsResponse(
            BillingResult billingResult,
            QueryProductDetailsResult queryResult) {
        if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
            productsLoaded(null, billingMessage("Could not load Google Play products", billingResult));
            return;
        }

        Map<String, ProductOffer> loadedProducts = new HashMap<>();
        for (ProductDetails productDetails : queryResult.getProductDetailsList()) {
            ProductDetails.OneTimePurchaseOfferDetails offerDetails = selectOffer(productDetails);
            if (offerDetails != null) {
                loadedProducts.put(
                        productDetails.getProductId(),
                        new ProductOffer(productDetails, offerDetails));
            }
        }

        products.clear();
        products.putAll(loadedProducts);

        JSONArray productValues = new JSONArray();
        try {
            for (String productId : configuredProductIds) {
                ProductOffer product = products.get(productId);
                if (product == null) {
                    continue;
                }

                JSONObject productValue = new JSONObject();
                productValue.put("id", productId);
                productValue.put("title", product.productDetails.getName());
                productValue.put("displayPrice", product.offerDetails.getFormattedPrice());
                productValues.put(productValue);
            }
        } catch (JSONException exception) {
            productsLoaded(null, "Could not format Google Play product data.");
            return;
        }

        if (productValues.length() == 0) {
            productsLoaded(null, "Google Play returned no configured tip products.");
            return;
        }
        productsLoaded(productValues.toString(), null);
    }

    private static ProductDetails.OneTimePurchaseOfferDetails selectOffer(ProductDetails productDetails) {
        List<ProductDetails.OneTimePurchaseOfferDetails> offers =
                productDetails.getOneTimePurchaseOfferDetailsList();
        if (offers != null && !offers.isEmpty()) {
            for (ProductDetails.OneTimePurchaseOfferDetails offer : offers) {
                if (offer.getOfferId() == null) {
                    return offer;
                }
            }
            return offers.get(0);
        }
        return productDetails.getOneTimePurchaseOfferDetails();
    }

    private static void launchBillingFlow(Activity activity, ProductOffer product) {
        BillingFlowParams.ProductDetailsParams.Builder productParams =
                BillingFlowParams.ProductDetailsParams.newBuilder()
                        .setProductDetails(product.productDetails);
        String offerToken = product.offerDetails.getOfferToken();
        if (offerToken != null && !offerToken.isEmpty()) {
            productParams.setOfferToken(offerToken);
        }

        BillingFlowParams flowParams = BillingFlowParams.newBuilder()
                .setProductDetailsParamsList(Collections.singletonList(productParams.build()))
                .build();
        BillingResult billingResult = billingClient.launchBillingFlow(activity, flowParams);
        if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
            finishPurchase(
                    PurchaseResult.Failed.toInt(),
                    billingMessage("Could not start the Google Play purchase", billingResult));
        }
    }

    private static void onPurchasesUpdated(
            BillingResult billingResult,
            List<Purchase> purchases) {
        boolean reportActivePurchase = purchaseCallbackPending;
        int responseCode = billingResult.getResponseCode();
        if (responseCode == BillingClient.BillingResponseCode.USER_CANCELED) {
            finishPurchase(PurchaseResult.Canceled.toInt(), null);
            return;
        }
        if (responseCode == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            queryPurchases(reportActivePurchase);
            return;
        }
        if (responseCode != BillingClient.BillingResponseCode.OK) {
            finishPurchase(
                    PurchaseResult.Failed.toInt(),
                    billingMessage("Google Play did not complete the purchase", billingResult));
            return;
        }
        if (!handlePurchases(purchases, reportActivePurchase) && reportActivePurchase) {
            finishPurchase(PurchaseResult.Failed.toInt(), "Google Play returned no matching purchase.");
        }
    }

    private static boolean handlePurchases(List<Purchase> purchases, boolean reportActivePurchase) {
        if (purchases == null) {
            return false;
        }

        boolean handled = false;
        for (Purchase purchase : purchases) {
            boolean matchesActiveProduct = activeProductId != null
                    && purchase.getProducts().contains(activeProductId);
            boolean isConfiguredTip = false;
            for (String productId : purchase.getProducts()) {
                if (configuredProductIds.contains(productId)) {
                    isConfiguredTip = true;
                    break;
                }
            }
            if (!isConfiguredTip || (reportActivePurchase && !matchesActiveProduct)) {
                continue;
            }

            handled = true;
            if (purchase.getPurchaseState() == Purchase.PurchaseState.PENDING) {
                if (reportActivePurchase) {
                    finishPurchase(PurchaseResult.Pending.toInt(), null);
                }
            } else if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                consumePurchase(purchase, reportActivePurchase);
            }
        }
        return handled;
    }

    private static void consumePurchase(Purchase purchase, boolean reportActivePurchase) {
        String purchaseToken = purchase.getPurchaseToken();
        if (!consumptionsInProgress.add(purchaseToken)) {
            if (reportActivePurchase) {
                finishPurchase(PurchaseResult.Failed.toInt(), "This Google Play purchase is already being processed.");
            }
            return;
        }

        ConsumeParams params = ConsumeParams.newBuilder()
                .setPurchaseToken(purchaseToken)
                .build();
        billingClient.consumeAsync(params, (billingResult, consumedToken) -> {
            consumptionsInProgress.remove(consumedToken);
            if (!reportActivePurchase) {
                if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                    Log.e(TAG, billingMessage("Could not consume an outstanding tip", billingResult));
                }
                return;
            }

            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                finishPurchase(PurchaseResult.Succeeded.toInt(), null);
            } else {
                finishPurchase(
                        PurchaseResult.Failed.toInt(),
                        billingMessage("Could not finish the Google Play purchase", billingResult));
            }
        });
    }

    private static void queryOutstandingPurchases() {
        queryPurchases(false);
    }

    private static void queryPurchases(boolean reportActivePurchase) {
        if (billingClient == null || !billingClient.isReady()) {
            if (reportActivePurchase) {
                finishPurchase(PurchaseResult.Failed.toInt(), "Google Play Billing is not connected.");
            }
            return;
        }

        QueryPurchasesParams params = QueryPurchasesParams.newBuilder()
                .setProductType(BillingClient.ProductType.INAPP)
                .build();
        billingClient.queryPurchasesAsync(params, (billingResult, purchases) -> {
            if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                if (reportActivePurchase) {
                    finishPurchase(
                            PurchaseResult.Failed.toInt(),
                            billingMessage("Could not query Google Play purchases", billingResult));
                }
                return;
            }
            if (!handlePurchases(purchases, reportActivePurchase) && reportActivePurchase) {
                finishPurchase(PurchaseResult.Failed.toInt(), "Google Play returned no matching purchase.");
            }
        });
    }

    private static void finishPurchase(int result, String errorMessage) {
        if (!purchaseCallbackPending) {
            return;
        }

        purchaseCallbackPending = false;
        activeProductId = null;
        purchaseFinished(result, errorMessage);
    }

    private static String billingMessage(String prefix, BillingResult billingResult) {
        String debugMessage = billingResult.getDebugMessage();
        if (debugMessage == null || debugMessage.isEmpty()) {
            return prefix + " (response " + billingResult.getResponseCode() + ").";
        }
        return prefix + " (response " + billingResult.getResponseCode() + "): " + debugMessage;
    }
}
