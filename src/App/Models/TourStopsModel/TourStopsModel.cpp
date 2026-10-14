#include "TourStopsModel.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "glog/logging.h"

TourStopsModel::TourStopsModel(QObject * parent)
    : QAbstractListModel(parent)
{
}

int TourStopsModel::rowCount(const QModelIndex & parent) const
{
    return static_cast<int>(m_stops.size());
}

QVariant TourStopsModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid())
    {
        LOG(ERROR) << "Invalid model index";
        return assert(false && "Invalid model index"), QVariant();
    }

    const auto & stop = m_stops.at(index.row());
    switch (role)
    {
        case Title:
            return stop.title;
        case Description:
            return stop.description;
        case ImageUrl:
            return stop.imageUrl;
        case Coords:
            return QVariant::fromValue(stop.coords);
        case SequenceNumber:
            return stop.sequenceNumber;
        case Selected:
            return stop.selected;
        default:
            return {};
    }
}

QHash<int, QByteArray> TourStopsModel::roleNames() const
{
#define ROLENAME(NAME)     \
    {                      \
        Roles::NAME, #NAME \
    }
    return {
        ROLENAME(Title),
        ROLENAME(Description),
        ROLENAME(ImageUrl),
        ROLENAME(Coords),
        ROLENAME(SequenceNumber),
        ROLENAME(Selected),
    };
#undef ROLENAME
}

void TourStopsModel::SetStops(QList<TourStop> stops)
{
    if (m_stops == stops)
        return;

    beginResetModel();
    m_stops = std::move(stops);
    m_stops.front().selected = true;
    endResetModel();
}

bool TourStopsModel::UpdateStop(int row, const TourStop & stop)
{
    if (row < 0 || row >= m_stops.size())
        return false;

    if (m_stops.at(row) == stop)
        return true;

    m_stops[row] = stop;

    emit dataChanged(index(row, 0), index(row, 0));
    return true;
}

QString TourStopsModel::GetSelectedStopTitle() const
{
    const auto it = std::ranges::find_if(m_stops, [&](decltype(m_stops)::const_reference stop) {
        return stop.selected;
    });
    assert(it != m_stops.cend());
    return it->title;
}

void TourStopsModel::SelectNextStop()
{
    const auto current = std::ranges::find_if(m_stops, &TourStop::selected);
    assert(current != m_stops.cend());

    const auto next = std::ranges::next(current);
    if (next == m_stops.cend())
        return;

    current->selected = false;
    next->selected = true;

    emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { Roles::Selected });
    emit SelectionChanged(*next);
}

void TourStopsModel::SelectPrevStop()
{
    const auto current = std::ranges::find_if(m_stops, &TourStop::selected);
    assert(current != m_stops.cend());
    if (current == m_stops.cbegin())
        return;

    const auto prev = std::ranges::prev(current);

    current->selected = false;
    prev->selected = true;

    emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { Roles::Selected });
    emit SelectionChanged(*prev);
}
