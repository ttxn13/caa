#include "pch.hpp"
#include "UtilJson.hpp"

namespace UtilJson {

bool read_json_object_file(const QString& file_path, QJsonObject& root_obj)
{
	QFile file(file_path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Error: cannot open json file:" << file_path;
		return false;
	}

	QJsonParseError parse_error;
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse_error);
	if (parse_error.error != QJsonParseError::NoError || !doc.isObject()) {
		qDebug() << "Error: cannot parse json object file:" << file_path << parse_error.errorString();
		return false;
	}

	root_obj = doc.object();
	return true;
}

bool read_json_object_file(const QString& file_path, QJsonObject* root_obj)
{
	if (root_obj == nullptr) return false;
	return read_json_object_file(file_path, *root_obj);
}

bool write_json_object_file(const QString& file_path, const QJsonObject& root_obj)
{
	const QFileInfo file_info(file_path);
	const QString dir_path = file_info.absolutePath();
	if (!dir_path.isEmpty() && !QDir().mkpath(dir_path)) {
		qDebug() << "Error: cannot create json dir:" << dir_path;
		return false;
	}

	QFile file(file_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
		qDebug() << "Error: cannot write json file:" << file_path;
		return false;
	}

	file.write(QJsonDocument(root_obj).toJson(QJsonDocument::Indented));
	return true;
}

QString json_text(const QJsonObject& obj, const QString& key, const QString& def)
{
	const QJsonValue value = obj.value(key);
	if (value.isString()) return value.toString().trimmed();
	if (value.isDouble()) return QString::number(value.toDouble());
	if (value.isBool()) return value.toBool() ? QStringLiteral("1") : QStringLiteral("0");
	return def;
}

}
