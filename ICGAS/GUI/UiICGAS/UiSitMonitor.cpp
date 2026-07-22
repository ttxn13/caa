#include "pch.hpp"
#include "UiSitMonitor.hpp"
#include "UiICGAS.hpp"
#include "../../Module/ModuleDatabase.hpp"

namespace {
const S_MOTION_FRAME* latest_motion(const S_PLAT& plat)
{
	return plat.list_motion.empty() ? nullptr : &plat.list_motion.back();
}

const S_MOTION_FRAME* latest_motion(const S_WEAPON& weapon)
{
	return weapon.list_motion.empty() ? nullptr : &weapon.list_motion.back();
}

const S_MOTION_FRAME* latest_motion(const S_FORMAT& format)
{
	return format.list_motion.empty() ? nullptr : &format.list_motion.back();
}

struct MonitorGroupBucket {
	QVariantMap row;
	QVariantList members;
};

QString row_string(const QVariantMap& row, const QString& key)
{
	return row.value(key).toString().trimmed();
}

bool is_row_kind(const QVariantMap& row, const QString& kind)
{
	return row_string(row, QStringLiteral("entityKind")) == kind;
}

int row_sort_rank(const QVariantMap& row)
{
	const int sort = row.value(QStringLiteral("threatSort")).toInt();
	return sort > 0 ? sort : 1000000000;
}

double row_threat_value(const QVariantMap& row)
{
	return row.value(QStringLiteral("threatValue")).toDouble();
}

bool threat_row_less(const QVariant& left, const QVariant& right)
{
	const QVariantMap left_row = left.toMap();
	const QVariantMap right_row = right.toMap();
	const int left_sort = row_sort_rank(left_row);
	const int right_sort = row_sort_rank(right_row);
	if (left_sort != right_sort) return left_sort < right_sort;
	const double left_threat = row_threat_value(left_row);
	const double right_threat = row_threat_value(right_row);
	if (!qFuzzyCompare(left_threat + 1.0, right_threat + 1.0)) return left_threat > right_threat;
	return row_string(left_row, QStringLiteral("displayId")) < row_string(right_row, QStringLiteral("displayId"));
}

void sort_threat_rows(QVariantList& rows)
{
	std::sort(rows.begin(), rows.end(), threat_row_less);
}

QString fmt_lookup_key(const QString& side_key, const QString& fmt_id)
{
	return side_key + QChar(0x1f) + fmt_id;
}

QVariantMap make_synthetic_group_row(const QString& side_key, const QString& fmt_id)
{
	QVariantMap row;
	row[QStringLiteral("groupId")] = QString();
	row[QStringLiteral("platId")] = QString();
	row[QStringLiteral("displayId")] = fmt_id.isEmpty() ? QStringLiteral("未分群") : fmt_id;
	row[QStringLiteral("fmtId")] = fmt_id;
	row[QStringLiteral("memberText")] = QString();
	row[QStringLiteral("sideKey")] = side_key;
	row[QStringLiteral("entityKind")] = QStringLiteral("syntheticGroup");
	row[QStringLiteral("entityKindText")] = QStringLiteral("分群");
	row[QStringLiteral("type")] = QStringLiteral("分群");
	row[QStringLiteral("typeName")] = QString();
	row[QStringLiteral("updateTime")] = QStringLiteral("--");
	row[QStringLiteral("threatValue")] = 0.0;
	row[QStringLiteral("threatValueText")] = QStringLiteral("--");
	row[QStringLiteral("threatLevel")] = 0;
	row[QStringLiteral("threatLevelText")] = QStringLiteral("--");
	row[QStringLiteral("threatSort")] = 0;
	row[QStringLiteral("threatSortText")] = QStringLiteral("--");
	row[QStringLiteral("cogValue")] = 0.0;
	row[QStringLiteral("cogValueText")] = QStringLiteral("--");
	row[QStringLiteral("cogLevel")] = 0;
	row[QStringLiteral("cogLevelText")] = QStringLiteral("--");
	row[QStringLiteral("cogSort")] = 0;
	row[QStringLiteral("cogSortText")] = QStringLiteral("--");
	row[QStringLiteral("fmtRangeText")] = QStringLiteral("--");
	row[QStringLiteral("formationRangeText")] = QStringLiteral("--");
	row[QStringLiteral("rangeText")] = QStringLiteral("--");
	return row;
}

QString stage_key(const E_STAGE stage)
{
	switch (stage) {
	case E_STAGE::WAN: return QStringLiteral("WAN");
	case E_STAGE::FLT: return QStringLiteral("FLT");
	case E_STAGE::COP: return QStringLiteral("COP");
	case E_STAGE::VSL: return QStringLiteral("VSL");
	default: return QStringLiteral("UKN");
	}
}

QString stage_text(const E_STAGE stage)
{
	switch (stage) {
	case E_STAGE::WAN: return QStringLiteral("预警区");
	case E_STAGE::FLT: return QStringLiteral("战斗机交战区");
	case E_STAGE::COP: return QStringLiteral("协同交战区");
	case E_STAGE::VSL: return QStringLiteral("舰艇自防区");
	default: return QStringLiteral("未知阶段");
	}
}

QString action_text(const E_ACT action)
{
	switch (action) {
	case E_ACT::DETECT: return QStringLiteral("探测");
	case E_ACT::TRACK: return QStringLiteral("跟踪");
	case E_ACT::ATTACK: return QStringLiteral("攻击");
	case E_ACT::GUIDE: return QStringLiteral("制导");
	default: return QStringLiteral("不明");
	}
}

QString action_list_text(const std::vector<E_ACT>& actions)
{
	QStringList action_texts;
	for (const E_ACT action : actions) {
		const QString text = action_text(action);
		if (!text.isEmpty()) action_texts.push_back(text);
	}
	return action_texts.isEmpty() ? QStringLiteral("--") : action_texts.join(QStringLiteral(" / "));
}

void normalize_group_row(QVariantMap& row)
{
	const QString group_id = row_string(row, QStringLiteral("platId"));
	QString display_id = row_string(row, QStringLiteral("displayId"));
	if (display_id.isEmpty()) display_id = row_string(row, QStringLiteral("fmtId"));

	row[QStringLiteral("groupId")] = group_id;
	row[QStringLiteral("displayId")] = display_id.isEmpty() ? QStringLiteral("未命名分群") : display_id;
	row[QStringLiteral("entityKindText")] = QStringLiteral("分群");
	QString range_text = row_string(row, QStringLiteral("formationRangeText"));
	if (range_text.isEmpty()) range_text = row_string(row, QStringLiteral("fmtRangeText"));
	row[QStringLiteral("rangeText")] = range_text.isEmpty() ? QStringLiteral("--") : range_text;
}

QVariantMap make_group_member_row(const QVariantMap& row)
{
	QVariantMap member;
	static const QStringList keys = {
		QStringLiteral("platId"),
		QStringLiteral("displayId"),
		QStringLiteral("type"),
		QStringLiteral("typeName"),
		QStringLiteral("updateTime"),
		QStringLiteral("threatValue"),
		QStringLiteral("threatValueText"),
		QStringLiteral("threatLevel"),
		QStringLiteral("threatLevelText"),
		QStringLiteral("threatSort"),
		QStringLiteral("threatSortText"),
		QStringLiteral("sideKey"),
		QStringLiteral("entityKind"),
		QStringLiteral("fmtId"),
		QStringLiteral("cmdId")
	};
	for (const QString& key : keys) {
		member.insert(key, row.value(key));
	}
	if (row_string(member, QStringLiteral("displayId")).isEmpty()) {
		member[QStringLiteral("displayId")] = row_string(member, QStringLiteral("platId"));
	}
	return member;
}
}

UiSitMonitor::UiSitMonitor(UiICGAS* backend, QObject* parent)
	: QObject(parent),
	ui_icgas(backend)
{
	rebuild_coa_rows();
}

QObject* UiSitMonitor::backend() const
{
	return ui_icgas;
}

void UiSitMonitor::setTrackLength(int length)
{
	const int next_length = std::clamp(length, 0, kMaxTrackCacheSize);
	if (track_length == next_length) return;
	track_length = next_length;
	emit trackLengthChanged();
	refreshSituation();
}

void UiSitMonitor::onAlgoPlatSnapshot(SNAPSHOT_ALGO_PLAT list_algo_plat)
{
	platform_snapshot.clear();
	if (!list_algo_plat.isNull()) {
		platform_snapshot = *list_algo_plat;
	}
	schedule_refresh();
}

void UiSitMonitor::onAlgoWeaponSnapshot(SNAPSHOT_ALGO_WEAPON list_algo_weapon)
{
	weapon_snapshot.clear();
	if (!list_algo_weapon.isNull()) {
		weapon_snapshot = *list_algo_weapon;
	}
	schedule_refresh();
}

void UiSitMonitor::onAlgoFmtSnapshot(SNAPSHOT_ALGO_FMT list_algo_fmt)
{
	format_snapshot.clear();
	if (!list_algo_fmt.isNull()) {
		format_snapshot = *list_algo_fmt;
	}
	schedule_refresh();
}

void UiSitMonitor::onAlgoCoaSnapshot(SNAPSHOT_ALGO_COA algo_coa)
{
	coa_snapshot = S_COA{};
	if (!algo_coa.isNull()) {
		coa_snapshot = *algo_coa;
	}
	rebuild_coa_rows();
	emit coaChanged();
}

void UiSitMonitor::refreshSituation()
{
	refresh_pending = false;
	if (ui_icgas == nullptr) return;

	QVariantList new_platform_rows;
	const std::map<QString, S_PLAT>& plat_raw = platform_snapshot;
	const std::map<QString, S_WEAPON>& weapon_raw = weapon_snapshot;
	const std::map<QString, S_FORMAT>& format_raw = format_snapshot;
	std::set<QString> active_track_ids;

	ui_icgas->monitor_platform_cache.clear();
	ui_icgas->monitor_weapon_cache.clear();
	ui_icgas->monitor_format_cache.clear();

	for (const auto& item : format_raw) {
		const S_FORMAT& format = item.second;
		if (!format.valid || format.fmt_id.trimmed().isEmpty()) continue;
		const S_MOTION_FRAME* motion = latest_motion(format);
		if (motion == nullptr) continue;

		QVariantMap row = ui_icgas->make_format_row(format);
		const QString monitor_id = row.value(QStringLiteral("platId")).toString();
		if (monitor_id.isEmpty()) continue;

		ui_icgas->monitor_format_cache[monitor_id] = format;
		remember_track_point(active_track_ids, monitor_id,
			motion->pos_lla.lon_rad RAD2DEG,
			motion->pos_lla.lat_rad RAD2DEG,
			motion->pos_lla.alt_m);

		row[QStringLiteral("route_points")] = QVariantList{};
		row[QStringLiteral("trail_points")] = sampled_track_points(monitor_id);
		new_platform_rows.push_back(row);
	}

	for (const auto& item : plat_raw) {
		const S_PLAT& plat = item.second;
		if (!plat.valid || plat.plt_id.trimmed().isEmpty()) continue;
		const S_MOTION_FRAME* motion = latest_motion(plat);
		if (motion == nullptr) continue;

		ui_icgas->monitor_platform_cache[plat.plt_id] = plat;
		remember_track_point(active_track_ids, plat.plt_id,
			motion->pos_lla.lon_rad RAD2DEG,
			motion->pos_lla.lat_rad RAD2DEG,
			motion->pos_lla.alt_m);

		QVariantMap row = ui_icgas->make_plat_row(plat);
		row[QStringLiteral("route_points")] = configured_route_points(plat.plt_id);
		row[QStringLiteral("trail_points")] = sampled_track_points(plat.plt_id);
		new_platform_rows.push_back(row);
	}

	for (const auto& item : weapon_raw) {
		const S_WEAPON& weapon = item.second;
		if (!weapon.valid || weapon.weapon_id.trimmed().isEmpty()) continue;
		const S_MOTION_FRAME* motion = latest_motion(weapon);
		if (motion == nullptr) continue;

		ui_icgas->monitor_weapon_cache[weapon.weapon_id] = weapon;
		remember_track_point(active_track_ids, weapon.weapon_id,
			motion->pos_lla.lon_rad RAD2DEG,
			motion->pos_lla.lat_rad RAD2DEG,
			motion->pos_lla.alt_m);

		QVariantMap row = ui_icgas->make_weapon_row(weapon);
		row[QStringLiteral("route_points")] = QVariantList{};
		row[QStringLiteral("trail_points")] = sampled_track_points(weapon.weapon_id);
		new_platform_rows.push_back(row);
	}

	prune_track_cache(active_track_ids);
	sync_backend_rows(new_platform_rows);

	const bool situation_changed = platform_rows != new_platform_rows;
	if (situation_changed) {
		platform_rows = std::move(new_platform_rows);
		rebuild_row_caches();
		emit situationChanged();
	}

	if (platform_rows.isEmpty()) {
		if (!ui_icgas->primary_plat_id.isEmpty() || !ui_icgas->secondary_plat_id.isEmpty()) {
			ui_icgas->primary_plat_id.clear();
			ui_icgas->secondary_plat_id.clear();
		}
	}
	else {
		auto exists = [this](const QString& id) {
			if (id.isEmpty()) return true;
			for (const QVariant& value : platform_rows) {
				if (value.toMap().value(QStringLiteral("platId")).toString() == id) return true;
			}
			return false;
		};
		if (!exists(ui_icgas->primary_plat_id)) ui_icgas->primary_plat_id.clear();
		if (!exists(ui_icgas->secondary_plat_id)) ui_icgas->secondary_plat_id.clear();
	}

	if (ui_icgas->refresh_detail()) {
		emit ui_icgas->detailChanged();
	}
}

QVariantMap UiSitMonitor::make_track_point(const TrackPoint& point) const
{
	QVariantMap track_point;
	track_point[QStringLiteral("lon_deg")] = point.lon_deg;
	track_point[QStringLiteral("lat_deg")] = point.lat_deg;
	track_point[QStringLiteral("altitude_m")] = point.altitude_m;
	return track_point;
}

void UiSitMonitor::rebuild_coa_rows()
{
	QVariantList rows;
	for (int stage_index = 0; stage_index < static_cast<int>(E_STAGE::COUNT); ++stage_index) {
		const E_STAGE stage = static_cast<E_STAGE>(stage_index);
		const S_COA::S_COA_STAGE& coa_stage = coa_snapshot.coa_of_stage[stage_index];

		QVariantList assignments;
		int serial = 1;
		for (const auto& own_item : coa_stage.own_fmt_act) {
			const QString own_fmt_id = own_item.first.trimmed();
			for (const S_ACT& act : own_item.second) {
				const QString tgt_fmt_id = act.tgt_fmt_id.trimmed();
				const QString actions = action_list_text(act.act_list);

				QVariantMap assignment;
				assignment[QStringLiteral("serialText")] = QString::number(serial++);
				assignment[QStringLiteral("stageKey")] = stage_key(stage);
				assignment[QStringLiteral("stageText")] = stage_text(stage);
				assignment[QStringLiteral("ownFmtId")] = own_fmt_id.isEmpty() ? QStringLiteral("--") : own_fmt_id;
				assignment[QStringLiteral("targetFmtId")] = tgt_fmt_id.isEmpty() ? QStringLiteral("--") : tgt_fmt_id;
				assignment[QStringLiteral("actionText")] = actions;
				assignment[QStringLiteral("assignmentText")] = QStringLiteral("%1 -> %2")
					.arg(assignment.value(QStringLiteral("ownFmtId")).toString(),
						assignment.value(QStringLiteral("targetFmtId")).toString());
				assignments.push_back(assignment);
			}
		}

		QVariantMap row;
		row[QStringLiteral("stageIndex")] = stage_index;
		row[QStringLiteral("stageKey")] = stage_key(stage);
		row[QStringLiteral("stageText")] = stage_text(stage);
		row[QStringLiteral("assignmentCount")] = static_cast<int>(assignments.size());
		row[QStringLiteral("assignments")] = assignments;
		rows.push_back(row);
	}
	coa_stage_rows = std::move(rows);
}

QVariantList UiSitMonitor::group_rows_for_side(const QString& side_key) const
{
	std::map<QString, MonitorGroupBucket> groups;
	std::map<QString, QString> fmt_to_group_id;

	for (const QVariant& value : platform_rows) {
		QVariantMap row = value.toMap();
		if (row_string(row, QStringLiteral("sideKey")) != side_key || !is_row_kind(row, QStringLiteral("format"))) {
			continue;
		}

		normalize_group_row(row);
		const QString group_id = row_string(row, QStringLiteral("groupId"));
		if (group_id.isEmpty()) continue;

		MonitorGroupBucket& bucket = groups[group_id];
		bucket.row = row;

		const QString fmt_id = row_string(row, QStringLiteral("fmtId"));
		if (!fmt_id.isEmpty()) {
			fmt_to_group_id[fmt_lookup_key(side_key, fmt_id)] = group_id;
		}
	}

	for (const QVariant& value : platform_rows) {
		const QVariantMap row = value.toMap();
		if (row_string(row, QStringLiteral("sideKey")) != side_key || !is_row_kind(row, QStringLiteral("platform"))) {
			continue;
		}

		const QString fmt_id = row_string(row, QStringLiteral("fmtId"));
		QString group_key;
		const auto fmt_iter = fmt_to_group_id.find(fmt_lookup_key(side_key, fmt_id));
		if (!fmt_id.isEmpty() && fmt_iter != fmt_to_group_id.end()) {
			group_key = fmt_iter->second;
		}
		else {
			group_key = QStringLiteral("SYNTH::%1::%2")
				.arg(side_key, fmt_id.isEmpty() ? QStringLiteral("__ungrouped__") : fmt_id);
			if (groups.find(group_key) == groups.end()) {
				MonitorGroupBucket bucket;
				bucket.row = make_synthetic_group_row(side_key, fmt_id);
				groups.emplace(group_key, std::move(bucket));
			}
		}

		groups[group_key].members.push_back(make_group_member_row(row));
	}

	QVariantList result;
	for (auto& item : groups) {
		MonitorGroupBucket& bucket = item.second;
		sort_threat_rows(bucket.members);

		QStringList member_ids;
		for (const QVariant& member_value : bucket.members) {
			const QString id = row_string(member_value.toMap(), QStringLiteral("displayId"));
			if (!id.isEmpty()) member_ids.push_back(id);
		}
		if (row_string(bucket.row, QStringLiteral("memberText")).isEmpty()) {
			bucket.row[QStringLiteral("memberText")] = member_ids.join(QStringLiteral(", "));
		}
		bucket.row[QStringLiteral("memberCount")] = bucket.members.size();
		bucket.row[QStringLiteral("memberRows")] = bucket.members;
		result.push_back(bucket.row);
	}

	sort_threat_rows(result);
	return result;
}

void UiSitMonitor::rebuild_row_caches()
{
	red_situation_rows.clear();
	blue_situation_rows.clear();
	for (const QVariant& value : platform_rows) {
		const QVariantMap row = value.toMap();
		const QString side_key = row_string(row, QStringLiteral("sideKey"));
		if (side_key == QStringLiteral("red")) {
			red_situation_rows.push_back(row);
		}
		else if (side_key == QStringLiteral("blue")) {
			blue_situation_rows.push_back(row);
		}
	}

	red_group_rows = group_rows_for_side(QStringLiteral("red"));
	blue_group_rows = group_rows_for_side(QStringLiteral("blue"));
}

void UiSitMonitor::schedule_refresh()
{
	if (refresh_pending) return;
	refresh_pending = true;
	QTimer::singleShot(0, this, [this]() {
		if (!refresh_pending) return;
		refresh_pending = false;
		refreshSituation();
	});
}

QVariantList UiSitMonitor::configured_route_points(const QString& platform_id) const
{
	if (ui_icgas == nullptr) return {};

	const S_PLAT_CONFIG* config = ui_icgas->find_sim_platform_config(platform_id);
	if (config == nullptr || config->init_route.point_list.size() < 2) return {};

	return ui_icgas->make_initial_platform_plot_row(*config)
		.value(QStringLiteral("route_points")).toList();
}

QVariantList UiSitMonitor::sampled_track_points(const QString& id) const
{
	QVariantList points;
	const auto iter = track_cache.find(id);
	if (iter == track_cache.end() || track_length <= 0) return points;

	const QVector<TrackPoint>& cache = iter->second;
	if (cache.isEmpty()) return points;

	const int cache_size = static_cast<int>(cache.size());
	const int effective_length = cache_size > 1 ? std::max(track_length, 2) : track_length;
	const int first_index = std::max(0, cache_size - effective_length);
	const int available_count = cache_size - first_index;
	const int sample_step = std::max(1,
		static_cast<int>(std::ceil(static_cast<double>(available_count) /
			static_cast<double>(kMaxRenderedTrackPoints))));

	for (int index = first_index; index < cache_size; index += sample_step) {
		points.push_back(make_track_point(cache.at(index)));
	}
	if (points.isEmpty() || points.back().toMap() != make_track_point(cache.back())) {
		points.push_back(make_track_point(cache.back()));
	}
	return points;
}

void UiSitMonitor::remember_track_point(std::set<QString>& active_ids, const QString& id,
	double lon_deg, double lat_deg, double altitude_m)
{
	const QString trimmed_id = id.trimmed();
	if (trimmed_id.isEmpty()) return;

	QVector<TrackPoint>& cache = track_cache[trimmed_id];
	const TrackPoint point{lon_deg, lat_deg, altitude_m};
	if (cache.isEmpty() ||
		cache.back().lon_deg != point.lon_deg ||
		cache.back().lat_deg != point.lat_deg ||
		cache.back().altitude_m != point.altitude_m) {
		cache.push_back(point);
	}
	while (cache.size() > kMaxTrackCacheSize) {
		cache.removeFirst();
	}
	active_ids.insert(trimmed_id);
}

void UiSitMonitor::prune_track_cache(const std::set<QString>& active_ids)
{
	for (auto iter = track_cache.begin(); iter != track_cache.end();) {
		if (active_ids.find(iter->first) == active_ids.end()) {
			iter = track_cache.erase(iter);
		}
		else {
			++iter;
		}
	}
}

void UiSitMonitor::sync_backend_rows(const QVariantList& rows)
{
	if (ui_icgas == nullptr) return;
	if (ui_icgas->platform_rows == rows) return;
	ui_icgas->platform_rows = rows;
	emit ui_icgas->situationChanged();
}
