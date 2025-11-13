#pragma once

#include <QGeoPositionInfoSource>
#include <QVariant>

#include "App/Models/BaseModel.h"

#include "App/Utils/NonCopyMovable.h"

class NearestObjectsModel
	: public BaseModel
{
	Q_OBJECT

public:
	explicit NearestObjectsModel(QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	NON_COPY_MOVABLE(NearestObjectsModel);

	~NearestObjectsModel();
};