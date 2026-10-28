package org.qtproject.PastViewer;

import android.app.Activity;
import android.util.Log;

import com.android.billingclient.api.AcknowledgePurchaseParams;
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

    private static final class OwnershipCheck {
        final String productId;
        final long requestId;

        OwnershipCheck(String productId, long requestId) {
            this.productId = productId;
            this.requestId = requestId;
        }
    }

    private static final class AcknowledgementWaiter {
        final Runnable onSuccess;
        final FailureHandler onFailure;

        AcknowledgementWaiter(Runnable onSuccess, FailureHandler onFailure) {
            this.onSuccess = onSuccess;
            this.onFailure = onFailure;
        }
    }

    private static BillingClient billingClient;
    private static boolean connecting;
    private static boolean billingConnectionStarting;
    private static Runnable pendingReadyAction;
    private static FailureHandler pendingFailureHandler;
    private static final List<OwnershipCheck> ownershipChecksWaitingForConnection = new ArrayList<>();
    private static final Map<String, List<AcknowledgementWaiter>> acknowledgementWaiters = new HashMap<>();
    private static final List<String> configuredProductIds = new ArrayList<>();
    private static final Map<String, ProductOffer> products = new HashMap<>();
    private static final Set<String> consumptionsInProgress = new HashSet<>();
    private static String activeProductId;
    private static boolean activePurchaseIsNonConsumable;
    private static boolean nonConsumableFlowLaunched;
    private static boolean purchaseCallbackPending;

    private GooglePlayBillingHelper() {}

    private static native void productsLoaded(String productsJson, String errorMessage);
    private static native void purchaseFinished(int result, String errorMessage);
    private static native void ownershipChecked(long requestId, boolean owned);

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
            activePurchaseIsNonConsumable = false;
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

        startBillingConnection();
    }

    private static void startBillingConnection() {
        if (billingClient == null || billingClient.isReady() || connecting || billingConnectionStarting) {
            return;
        }

        billingConnectionStarting = true;
        connecting = true;
        try {
            billingClient.startConnection(new BillingClientStateListener() {
                @Override
                public void onBillingSetupFinished(BillingResult billingResult) {
                    connecting = false;
                    if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                        failPendingRequest(billingMessage("Could not connect to Google Play Billing", billingResult));
                        failQueuedOwnershipChecks();
                        return;
                    }

                    Runnable action = pendingReadyAction;
                    pendingReadyAction = null;
                    pendingFailureHandler = null;
                    try {
                        if (action != null) {
                            action.run();
                        }
                        queryOutstandingPurchases();
                    } finally {
                        flushQueuedOwnershipChecks();
                    }
                }

                @Override
                public void onBillingServiceDisconnected() {
                    // Play may drop the connection while startConnection is still in
                    // progress, then reconnect. That is not an ownership result.
                    connecting = false;
                }
            });
        } finally {
            billingConnectionStarting = false;
        }
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
        int responseCode = billingResult.getResponseCode();
        if (responseCode == BillingClient.BillingResponseCode.OK
                || responseCode == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            acknowledgePurchasedTours(purchases);
        }
        if (purchaseCallbackPending && activePurchaseIsNonConsumable) {
            if (!nonConsumableFlowLaunched) {
                // The Play sheet is not up yet. This update belongs to an older purchase.
                if (responseCode == BillingClient.BillingResponseCode.OK
                        || responseCode == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
                    handlePurchases(purchases, false);
                }
                return;
            }
            if (!isActiveTourPurchaseUpdate(responseCode, purchases)) {
                // This update is another purchase, often a tip. Do not fail the tour
                // that is still in progress, and do consume that tip.
                deliverExistingPurchaseUpdate(billingResult, purchases, false);
                return;
            }
            if (responseCode == BillingClient.BillingResponseCode.OK
                    || responseCode == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
                handlePurchases(purchases, false);
            }
            handleActiveNonConsumablePurchase(billingResult, purchases);
            return;
        }

        deliverExistingPurchaseUpdate(billingResult, purchases, purchaseCallbackPending);
    }

    private static boolean isActiveTourPurchaseUpdate(int responseCode, List<Purchase> purchases) {
        if (responseCode == BillingClient.BillingResponseCode.OK) {
            return purchasesContainActiveProduct(purchases);
        }
        if (responseCode == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            // An empty list is Play's signal to query the launched purchase.
            // A list of other products is not this tour.
            return purchases == null || purchases.isEmpty() || purchasesContainActiveProduct(purchases);
        }
        return true;
    }

    private static boolean purchasesContainActiveProduct(List<Purchase> purchases) {
        return activeProductId != null && findPurchaseForProduct(purchases, activeProductId) != null;
    }

    private static void deliverExistingPurchaseUpdate(
            BillingResult billingResult,
            List<Purchase> purchases,
            boolean reportActivePurchase) {
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
        activePurchaseIsNonConsumable = false;
        nonConsumableFlowLaunched = false;
        purchaseFinished(result, errorMessage);
    }

    private static String billingMessage(String prefix, BillingResult billingResult) {
        String debugMessage = billingResult.getDebugMessage();
        if (debugMessage == null || debugMessage.isEmpty()) {
            return prefix + " (response " + billingResult.getResponseCode() + ").";
        }
        return prefix + " (response " + billingResult.getResponseCode() + "): " + debugMessage;
    }

    public static void purchaseNonConsumable(Activity activity, String productId) {
        if (activity == null || productId == null || productId.isEmpty()) {
            purchaseFinished(PurchaseResult.Failed.toInt(), "No Google Play product was selected.");
            return;
        }

        activity.runOnUiThread(() -> {
            if (purchaseCallbackPending) {
                purchaseFinished(PurchaseResult.Failed.toInt(), "Another Google Play purchase is already in progress.");
                return;
            }

            activeProductId = productId;
            activePurchaseIsNonConsumable = true;
            purchaseCallbackPending = true;
            runWhenReady(
                    activity,
                    () -> queryAndLaunchNonConsumable(activity, productId),
                    message -> finishPurchase(PurchaseResult.Failed.toInt(), message));
        });
    }

    public static void queryOwned(Activity activity, String productId, long requestId) {
        if (activity == null || productId == null || productId.isEmpty()) {
            ownershipChecked(requestId, false);
            return;
        }

        activity.runOnUiThread(() -> {
            try {
                ensureBillingClient(activity);
                queueOwnershipCheck(productId, requestId);
                if (billingClient != null && billingClient.isReady()) {
                    flushQueuedOwnershipChecks();
                    return;
                }

                // Do not take the single tip-connection waiter. A tour check and a
                // tip load can both wait, and a disconnect is not an ownership result.
                runWhenReady(activity, null, message -> Log.e(TAG, message));
                if (billingClient != null && billingClient.isReady()) {
                    flushQueuedOwnershipChecks();
                }
            } catch (RuntimeException exception) {
                removeOwnershipCheck(requestId);
                Log.e(TAG, "Could not query Google Play purchases", exception);
                ownershipChecked(requestId, false);
            }
        });
    }

    private static void queryAndLaunchNonConsumable(Activity activity, String productId) {
        if (billingClient == null || !billingClient.isReady()) {
            finishPurchase(PurchaseResult.Failed.toInt(), "Google Play Billing is not connected.");
            return;
        }
        if (configuredProductIds.contains(productId)) {
            finishPurchase(PurchaseResult.Failed.toInt(), "This tour is unavailable in Google Play.");
            return;
        }

        List<QueryProductDetailsParams.Product> productsToQuery = new ArrayList<>();
        productsToQuery.add(QueryProductDetailsParams.Product.newBuilder()
                .setProductId(productId)
                .setProductType(BillingClient.ProductType.INAPP)
                .build());
        QueryProductDetailsParams params = QueryProductDetailsParams.newBuilder()
                .setProductList(productsToQuery)
                .build();
        billingClient.queryProductDetailsAsync(params, (billingResult, queryResult) ->
                onNonConsumableProductDetails(activity, productId, billingResult, queryResult));
    }

    private static void onNonConsumableProductDetails(
            Activity activity,
            String productId,
            BillingResult billingResult,
            QueryProductDetailsResult queryResult) {
        if (!purchaseCallbackPending || !activePurchaseIsNonConsumable || !productId.equals(activeProductId)) {
            return;
        }
        if (billingResult == null
                || billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK
                || queryResult == null) {
            finishPurchase(
                    PurchaseResult.Failed.toInt(),
                    billingResult == null
                            ? "Could not load the Google Play product."
                            : billingMessage("Could not load the Google Play product", billingResult));
            return;
        }

        List<ProductDetails> productDetailsList = queryResult.getProductDetailsList();
        if (productDetailsList == null) {
            finishPurchase(PurchaseResult.Failed.toInt(), "This tour is unavailable in Google Play.");
            return;
        }

        ProductDetails selected = null;
        for (ProductDetails productDetails : productDetailsList) {
            if (productDetails != null && productId.equals(productDetails.getProductId())) {
                selected = productDetails;
                break;
            }
        }
        ProductDetails.OneTimePurchaseOfferDetails offer = selected == null ? null : selectOffer(selected);
        if (selected == null || offer == null) {
            finishPurchase(PurchaseResult.Failed.toInt(), "This tour is unavailable in Google Play.");
            return;
        }
        nonConsumableFlowLaunched = true;
        launchBillingFlow(activity, new ProductOffer(selected, offer));
    }

    private static void handleActiveNonConsumablePurchase(BillingResult billingResult, List<Purchase> purchases) {
        int responseCode = billingResult.getResponseCode();
        if (responseCode == BillingClient.BillingResponseCode.USER_CANCELED) {
            finishPurchase(PurchaseResult.Canceled.toInt(), null);
            return;
        }
        if (responseCode == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            queryActiveNonConsumableOwnership();
            return;
        }
        if (responseCode != BillingClient.BillingResponseCode.OK) {
            finishPurchase(
                    PurchaseResult.Failed.toInt(),
                    billingMessage("Google Play did not complete the purchase", billingResult));
            return;
        }
        settleActiveNonConsumablePurchase(purchases);
    }

    private static void queryActiveNonConsumableOwnership() {
        if (billingClient == null || !billingClient.isReady()) {
            finishPurchase(PurchaseResult.Failed.toInt(), "Google Play Billing is not connected.");
            return;
        }

        QueryPurchasesParams params = QueryPurchasesParams.newBuilder()
                .setProductType(BillingClient.ProductType.INAPP)
                .build();
        billingClient.queryPurchasesAsync(params, (billingResult, purchases) -> {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                handlePurchases(purchases, false);
            }
            if (!purchaseCallbackPending || !activePurchaseIsNonConsumable) {
                return;
            }
            if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                finishPurchase(
                        PurchaseResult.Failed.toInt(),
                        billingMessage("Could not query Google Play purchases", billingResult));
                return;
            }
            settleActiveNonConsumablePurchase(purchases);
        });
    }

    private static void settleActiveNonConsumablePurchase(List<Purchase> purchases) {
        Purchase purchase = findPurchaseForProduct(purchases, activeProductId);
        if (purchase == null) {
            finishPurchase(PurchaseResult.Failed.toInt(), "Google Play returned no matching purchase.");
            return;
        }
        if (isConfiguredTipPurchase(purchase)) {
            finishPurchase(PurchaseResult.Failed.toInt(), "This tour is unavailable in Google Play.");
            return;
        }
        if (purchase.getPurchaseState() == Purchase.PurchaseState.PENDING) {
            finishPurchase(PurchaseResult.Pending.toInt(), null);
            return;
        }
        if (purchase.getPurchaseState() != Purchase.PurchaseState.PURCHASED) {
            finishPurchase(PurchaseResult.Failed.toInt(), "Google Play returned no matching purchase.");
            return;
        }
        acknowledgePurchase(
                purchase,
                () -> finishPurchase(PurchaseResult.Succeeded.toInt(), null),
                message -> finishPurchase(PurchaseResult.Failed.toInt(), message));
    }

    private static void acknowledgePurchasedTours(List<Purchase> purchases) {
        if (purchases == null) {
            return;
        }
        for (Purchase purchase : purchases) {
            if (purchase == null
                    || purchase.getPurchaseState() != Purchase.PurchaseState.PURCHASED
                    || purchase.isAcknowledged()
                    || isConfiguredTipPurchase(purchase)) {
                continue;
            }
            acknowledgePurchase(purchase, null, message -> Log.e(TAG, message));
        }
    }

    private static void queryOwnedProduct(String productId, long requestId) {
        try {
            if (billingClient == null || !billingClient.isReady()) {
                // Not ready is not an ownership result. The Play purchase query runs
                // when the connection callback flushes this check.
                queueOwnershipCheck(productId, requestId);
                startBillingConnection();
                return;
            }

            QueryPurchasesParams params = QueryPurchasesParams.newBuilder()
                    .setProductType(BillingClient.ProductType.INAPP)
                    .build();
            billingClient.queryPurchasesAsync(params, (billingResult, purchases) ->
                    reportOwnedProduct(requestId, productId, billingResult, purchases));
        } catch (RuntimeException exception) {
            Log.e(TAG, "Could not query Google Play purchases", exception);
            ownershipChecked(requestId, false);
        }
    }

    private static void reportOwnedProduct(
            long requestId,
            String productId,
            BillingResult billingResult,
            List<Purchase> purchases) {
        try {
            if (billingResult == null || billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                ownershipChecked(requestId, false);
                return;
            }

            Purchase ownedPurchase = null;
            if (purchases != null) {
                for (Purchase purchase : purchases) {
                    if (purchase == null
                            || purchase.getProducts() == null
                            || purchase.getPurchaseState() != Purchase.PurchaseState.PURCHASED
                            || !purchase.getProducts().contains(productId)
                            || isConfiguredTipPurchase(purchase)) {
                        continue;
                    }
                    ownedPurchase = purchase;
                    break;
                }
            }
            if (ownedPurchase == null) {
                ownershipChecked(requestId, false);
                return;
            }

            // PURCHASED is the Play query result. Acknowledgement can fail without
            // taking the entitlement away.
            if (!ownedPurchase.isAcknowledged()) {
                acknowledgePurchase(ownedPurchase, null, message -> Log.e(TAG, message));
            }
            ownershipChecked(requestId, true);
        } catch (RuntimeException exception) {
            Log.e(TAG, "Could not read Google Play purchases", exception);
            ownershipChecked(requestId, false);
        }
    }

    private static void queueOwnershipCheck(String productId, long requestId) {
        for (OwnershipCheck check : ownershipChecksWaitingForConnection) {
            if (check.requestId == requestId) {
                return;
            }
        }
        ownershipChecksWaitingForConnection.add(new OwnershipCheck(productId, requestId));
    }

    private static void flushQueuedOwnershipChecks() {
        if (ownershipChecksWaitingForConnection.isEmpty()) {
            return;
        }
        List<OwnershipCheck> checks = new ArrayList<>(ownershipChecksWaitingForConnection);
        ownershipChecksWaitingForConnection.clear();
        for (OwnershipCheck check : checks) {
            queryOwnedProduct(check.productId, check.requestId);
        }
    }

    private static void failQueuedOwnershipChecks() {
        if (ownershipChecksWaitingForConnection.isEmpty()) {
            return;
        }
        List<OwnershipCheck> checks = new ArrayList<>(ownershipChecksWaitingForConnection);
        ownershipChecksWaitingForConnection.clear();
        for (OwnershipCheck check : checks) {
            ownershipChecked(check.requestId, false);
        }
    }

    private static void removeOwnershipCheck(long requestId) {
        ownershipChecksWaitingForConnection.removeIf(check -> check.requestId == requestId);
    }

    private static Purchase findPurchaseForProduct(List<Purchase> purchases, String productId) {
        if (purchases == null || productId == null) {
            return null;
        }
        for (Purchase purchase : purchases) {
            if (purchase != null
                    && purchase.getProducts() != null
                    && purchase.getProducts().contains(productId)) {
                return purchase;
            }
        }
        return null;
    }

    private static boolean isConfiguredTipPurchase(Purchase purchase) {
        if (purchase.getProducts() == null) {
            return false;
        }
        for (String productId : purchase.getProducts()) {
            if (configuredProductIds.contains(productId)) {
                return true;
            }
        }
        return false;
    }

    private static void acknowledgePurchase(Purchase purchase, Runnable onSuccess, FailureHandler onFailure) {
        if (purchase.isAcknowledged()) {
            if (onSuccess != null) {
                onSuccess.run();
            }
            return;
        }

        String token = purchase.getPurchaseToken();
        if (token == null || token.isEmpty() || billingClient == null) {
            if (onFailure != null) {
                onFailure.failed("Google Play returned a purchase without a token.");
            }
            return;
        }

        AcknowledgementWaiter waiter = new AcknowledgementWaiter(onSuccess, onFailure);
        List<AcknowledgementWaiter> waiters = acknowledgementWaiters.get(token);
        if (waiters != null) {
            waiters.add(waiter);
            return;
        }

        waiters = new ArrayList<>();
        waiters.add(waiter);
        acknowledgementWaiters.put(token, waiters);

        AcknowledgePurchaseParams params = AcknowledgePurchaseParams.newBuilder()
                .setPurchaseToken(token)
                .build();
        billingClient.acknowledgePurchase(params, billingResult -> {
            List<AcknowledgementWaiter> pendingWaiters = acknowledgementWaiters.remove(token);
            boolean acknowledged = billingResult != null
                    && billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK;
            if (pendingWaiters == null) {
                return;
            }
            for (AcknowledgementWaiter pendingWaiter : pendingWaiters) {
                if (acknowledged) {
                    if (pendingWaiter.onSuccess != null) {
                        pendingWaiter.onSuccess.run();
                    }
                } else if (pendingWaiter.onFailure != null) {
                    pendingWaiter.onFailure.failed(billingResult == null
                            ? "Could not acknowledge the Google Play purchase."
                            : billingMessage("Could not acknowledge the Google Play purchase", billingResult));
                }
            }
        });
    }
}
