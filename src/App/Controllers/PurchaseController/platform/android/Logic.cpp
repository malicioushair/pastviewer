#include "App/Controllers/PurchaseController/platform/Logic.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>

#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include "glog/logging.h"

namespace PlatformDependentLogic {

namespace {

constexpr auto QT_NATIVE = "org/qtproject/qt/android/QtNative";
constexpr auto ACTIVITY = "activity";
constexpr auto GOOGLE_PLAY_BILLING_HELPER = "org/qtproject/PastViewer/GooglePlayBillingHelper";

std::mutex callbackMutex;
TipProductsHandler tipProductsHandler;
PurchaseHandler purchaseHandler;
std::unordered_map<uint64_t, std::function<void(bool)>> ownershipHandlers;
uint64_t nextOwnershipRequestId = 1;

QJniObject Activity()
{
    return QJniObject::callStaticObjectMethod(
        QT_NATIVE,
        ACTIVITY,
        "()Landroid/app/Activity;");
}

QString FromJString(JNIEnv * env, jstring value)
{
    if (!value)
        return {};

    const auto length = env->GetStringLength(value);
    const auto * characters = env->GetStringChars(value, nullptr);
    if (!characters)
        return {};

    const auto result = QString::fromUtf16(
        reinterpret_cast<const char16_t *>(characters),
        length);
    env->ReleaseStringChars(value, characters);
    return result;
}

template <typename Callback, typename... Args>
void DispatchToQt(Callback callback, Args... args)
{
    if (!callback)
        return;

    auto invoke = [callback = std::move(callback), ... args = std::move(args)]() mutable {
        callback(std::move(args)...);
    };
    if (auto * application = QCoreApplication::instance())
        QMetaObject::invokeMethod(application, std::move(invoke), Qt::QueuedConnection);
    else
        invoke();
}

void FinishOwnership(uint64_t requestId, bool owned)
{
    std::function<void(bool)> handler;
    {
        const std::scoped_lock lock(callbackMutex);
        const auto it = ownershipHandlers.find(requestId);
        if (it == ownershipHandlers.end())
            return;

        handler = std::move(it->second);
        ownershipHandlers.erase(it);
    }

    DispatchToQt(std::move(handler), owned);
}

} // namespace

extern "C" JNIEXPORT void JNICALL Java_org_qtproject_PastViewer_GooglePlayBillingHelper_productsLoaded(
    JNIEnv * env,
    jclass,
    jstring productsJson,
    jstring errorMessage)
{
    TipProductsHandler handler;
    {
        const std::scoped_lock lock(callbackMutex);
        handler = std::move(tipProductsHandler);
    }

    QVector<TipProduct> products;
    auto error = FromJString(env, errorMessage);
    if (productsJson)
    {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(FromJString(env, productsJson).toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray())
            error = QStringLiteral("Could not parse Google Play product data: %1").arg(parseError.errorString());
        else
        {
            for (const auto & value : document.array())
            {
                const auto product = value.toObject();
                products.push_back({
                    .id = product.value(QStringLiteral("id")).toString(),
                    .title = product.value(QStringLiteral("title")).toString(),
                    .displayPrice = product.value(QStringLiteral("displayPrice")).toString(),
                });
            }
        }
    }

    DispatchToQt(std::move(handler), std::move(products), std::move(error));
}

extern "C" JNIEXPORT void JNICALL Java_org_qtproject_PastViewer_GooglePlayBillingHelper_purchaseFinished(
    JNIEnv * env,
    jclass,
    jint result,
    jstring errorMessage)
{
    PurchaseHandler handler;
    {
        const std::scoped_lock lock(callbackMutex);
        handler = std::move(purchaseHandler);
    }

    const auto purchaseResult = [result] {
        switch (result)
        {
            case 0:
                return PurchaseResult::Succeeded;
            case 1:
                return PurchaseResult::Cancelled;
            case 2:
                return PurchaseResult::Pending;
            default:
                return PurchaseResult::Failed;
        }
    }();
    DispatchToQt(std::move(handler), purchaseResult, FromJString(env, errorMessage));
}

extern "C" JNIEXPORT void JNICALL Java_org_qtproject_PastViewer_GooglePlayBillingHelper_ownershipChecked(
    JNIEnv *,
    jclass,
    jlong requestId,
    jboolean owned)
{
    FinishOwnership(static_cast<uint64_t>(requestId), static_cast<bool>(owned));
}

bool SupportsNativePurchases()
{
    return true;
}

void InitializeBilling(QStringList productIds)
{
    if (productIds.isEmpty())
        return;

    const auto activity = Activity();
    if (!activity.isValid())
    {
        LOG(ERROR) << "Failed to get Android activity while initializing Google Play Billing";
        return;
    }

    QJniObject::callStaticMethod<void>(
        GOOGLE_PLAY_BILLING_HELPER,
        "initialize",
        "(Landroid/app/Activity;Ljava/lang/String;)V",
        activity.object(),
        QJniObject::fromString(productIds.join(QLatin1Char(','))).object());

    QJniEnvironment env;
    if (env.checkAndClearExceptions())
        LOG(ERROR) << "Failed to initialize Google Play Billing";
}

void LoadTipProducts(QStringList productIds, TipProductsHandler handler)
{
    bool requestInProgress = false;
    {
        const std::scoped_lock lock(callbackMutex);
        requestInProgress = static_cast<bool>(tipProductsHandler);
        // Replace any in-flight waiter so the outstanding Play query delivers to the latest caller.
        tipProductsHandler = std::move(handler);
    }
    if (requestInProgress)
        return;

    const auto activity = Activity();
    if (!activity.isValid())
    {
        TipProductsHandler failedHandler;
        {
            const std::scoped_lock lock(callbackMutex);
            failedHandler = std::move(tipProductsHandler);
        }
        DispatchToQt(std::move(failedHandler), QVector<TipProduct> {}, QStringLiteral("Android activity is unavailable."));
        return;
    }

    QJniObject::callStaticMethod<void>(
        GOOGLE_PLAY_BILLING_HELPER,
        "loadProducts",
        "(Landroid/app/Activity;Ljava/lang/String;)V",
        activity.object(),
        QJniObject::fromString(productIds.join(QLatin1Char(','))).object());

    QJniEnvironment env;
    if (env.checkAndClearExceptions())
    {
        TipProductsHandler failedHandler;
        {
            const std::scoped_lock lock(callbackMutex);
            failedHandler = std::move(tipProductsHandler);
        }
        DispatchToQt(std::move(failedHandler), QVector<TipProduct> {}, QStringLiteral("Could not call Google Play Billing."));
    }
}

void Purchase(QString productId, PurchaseHandler handler, bool nonConsumable)
{
    bool purchaseInProgress = false;
    {
        const std::scoped_lock lock(callbackMutex);
        if (purchaseHandler)
            purchaseInProgress = true;
        else
            purchaseHandler = std::move(handler);
    }
    if (purchaseInProgress)
    {
        handler(PurchaseResult::Failed, QStringLiteral("A Google Play purchase is already in progress."));
        return;
    }

    const auto activity = Activity();
    if (!activity.isValid())
    {
        PurchaseHandler failedHandler;
        {
            const std::scoped_lock lock(callbackMutex);
            failedHandler = std::move(purchaseHandler);
        }
        DispatchToQt(std::move(failedHandler), PurchaseResult::Failed, QStringLiteral("Android activity is unavailable."));
        return;
    }

    QJniObject::callStaticMethod<void>(
        GOOGLE_PLAY_BILLING_HELPER,
        nonConsumable ? "purchaseNonConsumable" : "purchaseProduct",
        "(Landroid/app/Activity;Ljava/lang/String;)V",
        activity.object(),
        QJniObject::fromString(productId).object());

    QJniEnvironment env;
    if (env.checkAndClearExceptions())
    {
        PurchaseHandler failedHandler;
        {
            const std::scoped_lock lock(callbackMutex);
            failedHandler = std::move(purchaseHandler);
        }
        DispatchToQt(std::move(failedHandler), PurchaseResult::Failed, QStringLiteral("Could not call Google Play Billing."));
    }
}

void CheckOwned(const QString & productId, std::function<void(bool)> handler)
{
    if (!handler)
        return;

    if (productId.isEmpty())
    {
        handler(false);
        return;
    }

    uint64_t requestId = 0;
    {
        const std::scoped_lock lock(callbackMutex);
        requestId = nextOwnershipRequestId++;
        ownershipHandlers.emplace(requestId, std::move(handler));
    }

    const auto activity = Activity();
    if (!activity.isValid())
    {
        LOG(ERROR) << "Failed to get Android activity while checking Google Play ownership";
        FinishOwnership(requestId, false);
        return;
    }

    QJniObject::callStaticMethod<void>(
        GOOGLE_PLAY_BILLING_HELPER,
        "queryOwned",
        "(Landroid/app/Activity;Ljava/lang/String;J)V",
        activity.object(),
        QJniObject::fromString(productId).object(),
        static_cast<jlong>(requestId));

    QJniEnvironment env;
    if (env.checkAndClearExceptions())
    {
        LOG(ERROR) << "Failed to query Google Play ownership";
        FinishOwnership(requestId, false);
    }
}

void QueryPurchasePending(const QString &, std::function<void(PurchasePendingState)> handler)
{
    if (handler)
        handler(PurchasePendingState::Unknown);
}

void SetVerifiedTransactionHandler(std::function<void(QString)>)
{
}

void SetPurchaseEndedHandler(std::function<void(QString, bool, QString)>)
{
}

} // namespace PlatformDependentLogic
