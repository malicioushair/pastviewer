#include "PastViewModelController.h"

#include <memory>

#include "App/Models/PastVuModel.h"

struct PastVuModelController::Impl
{
	std::unique_ptr<PastVuModel> pastVuModel { std::make_unique<PastVuModel>() };
};

PastVuModelController::PastVuModelController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>())
{
}

PastVuModelController::~PastVuModelController() = default;

QAbstractListModel * PastVuModelController::GetModel()
{
	return m_impl->pastVuModel.get();
}

QString PastVuModelController::GetMapHostApiKey()
{
	return QString::fromUtf8(API_KEY);
}
