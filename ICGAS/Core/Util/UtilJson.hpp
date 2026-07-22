#pragma once
#include "pch.hpp"

namespace UtilJson {

	bool read_json_object_file(const QString& file_path, QJsonObject& root_obj);
	bool read_json_object_file(const QString& file_path, QJsonObject* root_obj);
	bool write_json_object_file(const QString& file_path, const QJsonObject& root_obj);
	QString json_text(const QJsonObject& obj, const QString& key, const QString& def = QString());

}
