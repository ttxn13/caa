#pragma once
#include "pch.hpp"
#include "../../Module/ModuleDatabase.hpp"

class UiICGAS;

class UiSitMonitor : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QObject* backend READ backend CONSTANT)
	Q_PROPERTY(QVariantList platformRows READ platformRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList redSituationRows READ redSituationRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList blueSituationRows READ blueSituationRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList redGroupRows READ redGroupRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList blueGroupRows READ blueGroupRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList coaStageRows READ coaStageRows NOTIFY coaChanged)
	Q_PROPERTY(int trackLength READ trackLength WRITE setTrackLength NOTIFY trackLengthChanged)

public:
	explicit UiSitMonitor(UiICGAS* backend, QObject* parent = nullptr);

	QObject* backend() const;
	QVariantList platformRows() const { return platform_rows; }
	QVariantList redSituationRows() const { return red_situation_rows; }
	QVariantList blueSituationRows() const { return blue_situation_rows; }
	QVariantList redGroupRows() const { return red_group_rows; }
	QVariantList blueGroupRows() const { return blue_group_rows; }
	QVariantList coaStageRows() const { return coa_stage_rows; }
	int trackLength() const { return track_length; }
	void setTrackLength(int length);

	Q_INVOKABLE void refreshSituation();

public slots:
	void onAlgoPlatSnapshot(SNAPSHOT_ALGO_PLAT list_algo_plat);
	void onAlgoWeaponSnapshot(SNAPSHOT_ALGO_WEAPON list_algo_weapon);
	void onAlgoFmtSnapshot(SNAPSHOT_ALGO_FMT list_algo_fmt);
	void onAlgoCoaSnapshot(SNAPSHOT_ALGO_COA algo_coa);

signals:
	void situationChanged();
	void coaChanged();
	void trackLengthChanged();

private:
	static constexpr int kMaxTrackCacheSize = 240;
	static constexpr int kMaxRenderedTrackPoints = 36;

	struct TrackPoint {
		double lon_deg = 0.0;
		double lat_deg = 0.0;
		double altitude_m = 0.0;
	};

	QVariantMap make_track_point(const TrackPoint& point) const;
	void rebuild_coa_rows();
	QVariantList group_rows_for_side(const QString& side_key) const;
	void rebuild_row_caches();
	void schedule_refresh();
	QVariantList configured_route_points(const QString& platform_id) const;
	QVariantList sampled_track_points(const QString& id) const;
	void remember_track_point(std::set<QString>& active_ids, const QString& id,
		double lon_deg, double lat_deg, double altitude_m);
	void prune_track_cache(const std::set<QString>& active_ids);
	void sync_backend_rows(const QVariantList& rows);

	UiICGAS* ui_icgas = nullptr;
	std::map<QString, S_PLAT> platform_snapshot;
	std::map<QString, S_WEAPON> weapon_snapshot;
	std::map<QString, S_FORMAT> format_snapshot;
	S_COA coa_snapshot;
	QVariantList platform_rows;
	QVariantList red_situation_rows;
	QVariantList blue_situation_rows;
	QVariantList red_group_rows;
	QVariantList blue_group_rows;
	QVariantList coa_stage_rows;
	std::map<QString, QVector<TrackPoint>> track_cache;
	int track_length = 24;
	bool refresh_pending = false;
};
