#pragma once

#include <memory>

#include <QObject>
#include <QVariantList>

#include "App/Tours/DisplayTours.h"

namespace PastViewer {
class TipPromptTracker;
}

class PurchaseController
	: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList tipProducts READ TipProducts NOTIFY tipProductsChanged)
	Q_PROPERTY(bool tipProductsLoading READ TipProductsLoading NOTIFY tipProductsLoadingChanged)
	Q_PROPERTY(bool purchaseInProgress READ PurchaseInProgress NOTIFY purchaseInProgressChanged)

public:
	explicit PurchaseController(QObject * parent = nullptr);
	~PurchaseController() override;

	void SetTipPromptTracker(PastViewer::TipPromptTracker * tracker);

signals:
	void showErrorDialog(const QString & errorMessage);
	void tipJarRequested();
	void tipProductsChanged();
	void tipProductsLoadingChanged();
	void purchaseInProgressChanged();
	void tipPurchaseSucceeded();
	void tipPurchasePending();
	void tipOperationFailed();
	void tourPurchaseSucceeded(const QString & tourId);
	void tourPurchasePending(const QString & tourId);
	void tourPurchasePendingCleared(const QString & tourId);
	void tourOwned(const QString & tourId, bool owned);

public:
	Q_INVOKABLE bool HasTipSupport() const;
	Q_INVOKABLE void RequestTipFlow();
	Q_INVOKABLE void RequestTourPurchaseFlow(const DisplayTour & tour);
	Q_INVOKABLE bool IsTourPurchasePending(const QString & tourId) const;
	Q_INVOKABLE void LoadTipProducts();
	Q_INVOKABLE void PurchaseTip(const QString & productId);
	Q_INVOKABLE void CheckTourOwned(const DisplayTour & tour);

	QVariantList TipProducts() const;
	bool TipProductsLoading() const;
	bool PurchaseInProgress() const;

private:
	void NotePendingTourPurchase(const QString & productId, const QString & tourId);
	void ClearPendingTourMessage(const QString & productId);
	void CompletePendingTourPurchase(const QString & productId);
	void FailTourPurchase(const QString & productId, const QString & tourId, const QString & errorMessage);
	void SettlePendingTourPurchase(const QString & productId, const QString & tourId, int generation, bool owned);
	void RefreshPendingTourPurchases();
	void StartTourPurchase(const QString & tourId, const QString & productId);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
