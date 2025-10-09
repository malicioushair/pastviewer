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
	const auto a = 0;
}

PastVuModelController::~PastVuModelController() = default;

QAbstractListModel * PastVuModelController::GetModel()
{
	return m_impl->pastVuModel.get();
}

std::string PastVuModelController::GetMapHostApiKey()
{
	return API_KEY;
}
