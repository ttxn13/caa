#include "pch.hpp"
#include "../GUI/UiLaunch/UiLaunch.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>
#include <QStringList>

#include <osg/DisplaySettings>
#include <osg/Referenced>
#include <osg/Shader>
#include <osg/ref_ptr>
#include <osgDB/Registry>
#include <osgEarth/Common>
#include <osgEarth/Registry>
#include <osgEarth/ShaderFactory>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <delayimp.h>

// 作战行动方案智能生成与评估软件
// ICGAS: Intelligent Course of Action Generation and Assessment Software

namespace {
osg::ref_ptr<osg::Referenced> shader_compatibility_host;
std::vector<DLL_DIRECTORY_COOKIE> dll_directory_cookies;
QString pending_osgearth_app_dir;
bool osgearth_runtime_initialized = false;
constexpr int kMaxPreloadedRuntimeDirs = 8;
constexpr int kMaxPreloadedRuntimeDirChars = 1024;
wchar_t preloaded_runtime_dirs[kMaxPreloadedRuntimeDirs][kMaxPreloadedRuntimeDirChars] = {};
int preloaded_runtime_dir_count = 0;

void force_compatibility_glsl_header(osg::Shader* shader, osg::Referenced*)
{
	if (!shader) return;

	std::string source = shader->getShaderSource();
	constexpr char version_directive[] = "#version";
	const std::size_t bom_size =
		source.size() >= 3 &&
		static_cast<unsigned char>(source[0]) == 0xEF &&
		static_cast<unsigned char>(source[1]) == 0xBB &&
		static_cast<unsigned char>(source[2]) == 0xBF ? 3 : 0;

	std::size_t version_pos = source.find(version_directive, bom_size);
	while (version_pos != std::string::npos) {
		const std::size_t line_start = source.rfind('\n', version_pos);
		const std::size_t directive_line_start = line_start == std::string::npos ? 0 : line_start + 1;
		const bool starts_directive = std::all_of(
			source.begin() + static_cast<std::ptrdiff_t>(directive_line_start),
			source.begin() + static_cast<std::ptrdiff_t>(version_pos),
			[](char ch) { return ch == ' ' || ch == '\t' || ch == '\r'; });
		const char next = version_pos + 8 < source.size() ? source[version_pos + 8] : '\0';
		if (starts_directive && (next == ' ' || next == '\t')) {
			break;
		}
		version_pos = source.find(version_directive, version_pos + 8);
	}
	if (version_pos == std::string::npos) return;

	const std::size_t line_start = source.rfind('\n', version_pos);
	const std::size_t directive_line_start = line_start == std::string::npos ? 0 : line_start + 1;
	const std::size_t line_end = source.find('\n', version_pos);
	const std::size_t directive_end = line_end == std::string::npos ? source.size() : line_end;
	std::string line = source.substr(version_pos, directive_end - version_pos);
	if (!line.empty() && line.back() == '\r') {
		line.pop_back();
	}
	if (line.find(" core") != std::string::npos ||
		line.find(" compatibility") != std::string::npos ||
		line.find(" es") != std::string::npos) {
		// Preserve the declared profile, but still move #version to the first line.
	}
	else {
		const int version = std::atoi(line.c_str() + 8);
		if (version >= 150) {
			line += " compatibility";
		}
	}

	const std::size_t erase_end = line_end == std::string::npos ? source.size() : line_end + 1;
	std::string rest = source;
	rest.erase(directive_line_start, erase_end - directive_line_start);
	if (bom_size > 0 && rest.size() >= bom_size) {
		rest.erase(0, bom_size);
	}

	const std::string fixed_source = line + "\n" + rest;
	if (fixed_source != source) {
		shader->setShaderSource(fixed_source);
	}
}

QString app_dir_from_module(const char* argv0)
{
	wchar_t module_path[MAX_PATH] = {};
	const DWORD length = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
	if (length > 0 && length < MAX_PATH) {
		return QFileInfo(QString::fromWCharArray(module_path)).absolutePath();
	}

	const QFileInfo argv_info(QString::fromLocal8Bit(argv0 ? argv0 : ""));
	if (!argv_info.filePath().isEmpty()) {
		return argv_info.absolutePath();
	}

	return QDir::currentPath();
}

QString osgearth_bundle_config()
{
#ifdef _DEBUG
	return QStringLiteral("debug");
#else
	return QStringLiteral("release");
#endif
}

const wchar_t* osgearth_bundle_config_w()
{
#ifdef _DEBUG
	return L"debug";
#else
	return L"release";
#endif
}

bool is_osgearth_delay_dll(const char* dll_name)
{
	if (dll_name == nullptr) return false;
#ifdef _DEBUG
	const char* names[] = {
		"osgEarthd.dll",
		"osgViewerd.dll",
		"osgGAd.dll",
		"osgDBd.dll",
		"osgUtild.dll",
		"osgTextd.dll",
		"osgd.dll"
	};
#else
	const char* names[] = {
		"osgEarth.dll",
		"osgViewer.dll",
		"osgGA.dll",
		"osgDB.dll",
		"osgUtil.dll",
		"osgText.dll",
		"osg.dll"
	};
#endif
	for (const char* name : names) {
		if (_stricmp(dll_name, name) == 0) return true;
	}
	return false;
}

std::wstring executable_directory_w()
{
	wchar_t module_path[MAX_PATH] = {};
	const DWORD length = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
	if (length == 0 || length >= MAX_PATH) return {};

	std::wstring path(module_path, length);
	const std::size_t slash = path.find_last_of(L"\\/");
	if (slash == std::wstring::npos) return {};
	path.resize(slash);
	return path;
}

std::wstring widen_dll_name(const char* dll_name)
{
	if (dll_name == nullptr) return {};
	const int required = MultiByteToWideChar(CP_ACP, 0, dll_name, -1, nullptr, 0);
	if (required <= 1) return {};

	std::wstring result(static_cast<std::size_t>(required - 1), L'\0');
	MultiByteToWideChar(CP_ACP, 0, dll_name, -1, result.data(), required);
	return result;
}

std::wstring join_path_w(const std::wstring& base, const std::wstring& child)
{
	if (base.empty()) return child;
	if (base.back() == L'\\' || base.back() == L'/') return base + child;
	return base + L"\\" + child;
}

HMODULE load_from_directory_w(const std::wstring& directory, const std::wstring& dll_name)
{
	if (directory.empty() || dll_name.empty()) return nullptr;
	const std::wstring path = join_path_w(directory, dll_name);
	HMODULE module = LoadLibraryExW(
		path.c_str(),
		nullptr,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	if (!module) {
		module = LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
	}
	return module;
}

bool runtime_dir_already_preloaded(const std::wstring& runtime_dir)
{
	for (int i = 0; i < preloaded_runtime_dir_count; ++i) {
		if (_wcsicmp(preloaded_runtime_dirs[i], runtime_dir.c_str()) == 0) {
			return true;
		}
	}
	return false;
}

void remember_preloaded_runtime_dir(const std::wstring& runtime_dir)
{
	if (preloaded_runtime_dir_count >= kMaxPreloadedRuntimeDirs) return;

	wcsncpy_s(preloaded_runtime_dirs[preloaded_runtime_dir_count],
		kMaxPreloadedRuntimeDirChars,
		runtime_dir.c_str(),
		_TRUNCATE);
	++preloaded_runtime_dir_count;
}

void preload_osgearth_runtime_dependencies_w(const std::wstring& runtime_dir)
{
	if (runtime_dir.empty()) return;
	if (runtime_dir_already_preloaded(runtime_dir)) {
		return;
	}
	remember_preloaded_runtime_dir(runtime_dir);

#ifdef _DEBUG
	const wchar_t* dll_names[] = {
		L"zd.dll",
		L"zstd.dll",
		L"liblzma.dll",
		L"jpeg62.dll",
		L"libwebp.dll",
		L"libwebpdecoder.dll",
		L"libwebpdemux.dll",
		L"libwebpmux.dll",
		L"libpng16d.dll",
		L"Lerc.dll",
		L"sqlite3.dll",
		L"tiffd.dll",
		L"geotiff_d.dll",
		L"proj_9_d.dll",
		L"geos_c.dll",
		L"libcurl-d.dll",
		L"gdald.dll"
	};
#else
	const wchar_t* dll_names[] = {
		L"z.dll",
		L"zstd.dll",
		L"liblzma.dll",
		L"jpeg62.dll",
		L"libwebp.dll",
		L"libwebpdecoder.dll",
		L"libwebpdemux.dll",
		L"libwebpmux.dll",
		L"libpng16.dll",
		L"Lerc.dll",
		L"sqlite3.dll",
		L"tiff.dll",
		L"geotiff.dll",
		L"proj_9.dll",
		L"geos_c.dll",
		L"libcurl.dll",
		L"gdal.dll"
	};
#endif

	for (const wchar_t* dll_name : dll_names) {
		load_from_directory_w(runtime_dir, dll_name);
	}
}

HMODULE load_osgearth_delay_dll(const char* dll_name)
{
	if (!is_osgearth_delay_dll(dll_name)) return nullptr;

	const std::wstring dll_name_w = widen_dll_name(dll_name);
	const std::wstring exe_dir = executable_directory_w();
	const std::wstring runtime_relative =
		std::wstring(L"OsgEarth\\plugin\\") + osgearth_bundle_config_w() + L"\\bin";

	const std::wstring exe_runtime = join_path_w(exe_dir, runtime_relative);
	preload_osgearth_runtime_dependencies_w(exe_runtime);
	if (HMODULE module = load_from_directory_w(exe_runtime, dll_name_w)) {
		return module;
	}

	const std::wstring repo_runtime = join_path_w(
		join_path_w(exe_dir, L"..\\ICGAS"),
		runtime_relative);
	preload_osgearth_runtime_dependencies_w(repo_runtime);
	if (HMODULE module = load_from_directory_w(repo_runtime, dll_name_w)) {
		return module;
	}

	return nullptr;
}

FARPROC WINAPI delay_load_notify_hook(unsigned dli_notify, PDelayLoadInfo delay_info)
{
	if (dli_notify != dliNotePreLoadLibrary || delay_info == nullptr) return nullptr;
	return reinterpret_cast<FARPROC>(load_osgearth_delay_dll(delay_info->szDll));
}

}

ExternC const PfnDliHook __pfnDliNotifyHook2 = delay_load_notify_hook;

namespace {

bool path_entry_is_osgearth_conflict(const QString& entry)
{
	const QString clean = QDir::toNativeSeparators(QDir::cleanPath(entry)).toLower();
	return clean.contains(QStringLiteral("\\vcpkg\\installed\\x64-windows\\bin")) ||
		clean.contains(QStringLiteral("\\office\\graphviz\\bin")) ||
		clean.contains(QStringLiteral("\\tool\\perl\\c\\bin"));
}

void append_unique_path_entry(QStringList& entries, const QString& entry)
{
	const QString clean = QDir::toNativeSeparators(QDir::cleanPath(entry));
	if (clean.isEmpty()) return;
	for (const QString& existing : entries) {
		if (existing.compare(clean, Qt::CaseInsensitive) == 0) return;
	}
	entries.push_back(clean);
}

void prepend_path_entries(const QStringList& entries)
{
	QStringList normalized_entries;
	for (const QString& entry : entries) {
		append_unique_path_entry(normalized_entries, entry);
	}

	const QByteArray current_path = qgetenv("PATH");
	if (!current_path.isEmpty()) {
		const QStringList inherited_entries = QString::fromLocal8Bit(current_path).split(QChar(';'), Qt::SkipEmptyParts);
		for (const QString& entry : inherited_entries) {
			if (!path_entry_is_osgearth_conflict(entry)) {
				append_unique_path_entry(normalized_entries, entry);
			}
		}
	}

	const QString path = normalized_entries.join(QChar(';'));
	qputenv("PATH", path.toLocal8Bit());
	SetEnvironmentVariableA("PATH", path.toLocal8Bit().constData());
}

void add_dll_directory(const QString& path)
{
	const QString native_path = QDir::toNativeSeparators(QDir::cleanPath(path));
	if (native_path.isEmpty()) return;
	const DLL_DIRECTORY_COOKIE cookie = AddDllDirectory(reinterpret_cast<PCWSTR>(native_path.utf16()));
	if (cookie) dll_directory_cookies.push_back(cookie);
}

void set_process_environment(const char* name, const QString& value)
{
	const QString native_value = QDir::toNativeSeparators(QDir::cleanPath(value));
	qputenv(name, native_value.toLocal8Bit());
	SetEnvironmentVariableA(name, native_value.toLocal8Bit().constData());
}

void set_osg_library_path(const QString& path)
{
	const QString native_path = QDir::toNativeSeparators(QDir::cleanPath(path));
	if (native_path.isEmpty() || !QDir(native_path).exists()) return;

	auto& paths = osgDB::Registry::instance()->getLibraryFilePathList();
	paths.clear();
	const std::string value = native_path.toStdString();
	paths.push_back(value);
}

void configure_osgearth_paths(const QString& app_dir, bool preload_runtime_dependencies)
{
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
		LOAD_LIBRARY_SEARCH_SYSTEM32 |
		LOAD_LIBRARY_SEARCH_USER_DIRS);

	const QDir app(app_dir);
	const QString qt_bin_dir = QLibraryInfo::path(QLibraryInfo::BinariesPath);
	const QDir plugin_root(app.filePath(QStringLiteral("OsgEarth/plugin")));
	const QDir runtime_root(plugin_root.filePath(osgearth_bundle_config()));
	const QString bin_dir = runtime_root.filePath(QStringLiteral("bin"));
	const QString osg_plugin_dir = runtime_root.filePath(QStringLiteral("osgPlugins-3.6.5"));
	const QString gdal_data_dir = plugin_root.filePath(QStringLiteral("share/gdal"));
	const QString proj_data_dir = plugin_root.filePath(QStringLiteral("share/proj"));

	add_dll_directory(qt_bin_dir);
	add_dll_directory(bin_dir);
	add_dll_directory(app.absolutePath());
	prepend_path_entries(QStringList{ qt_bin_dir, bin_dir, app.absolutePath() });
	if (preload_runtime_dependencies) {
		preload_osgearth_runtime_dependencies_w(QDir::toNativeSeparators(QDir::cleanPath(bin_dir)).toStdWString());
	}

	set_process_environment("OSG_LIBRARY_PATH", osg_plugin_dir);
	set_process_environment("GDAL_DATA", gdal_data_dir);
	set_process_environment("PROJ_DATA", proj_data_dir);
	set_process_environment("PROJ_LIB", proj_data_dir);
	if (preload_runtime_dependencies) {
		set_osg_library_path(osg_plugin_dir);
	}
}

void ensure_osgearth_runtime_initialized_impl();

void prepare_osgearth_runtime(const QString& app_dir)
{
	_putenv_s("OSG_GL_CONTEXT_VERSION", "3.3");
	_putenv_s("OSG_GL_CONTEXT_PROFILE_MASK", "2");
	_putenv_s("OSG_SHADER", "HINT=GL2");

#ifdef Q_OS_WIN
	qputenv("QSG_RHI_BACKEND", "d3d11");
	QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
#else
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

	QSurfaceFormat format = QSurfaceFormat::defaultFormat();
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CompatibilityProfile);
	format.setOption(QSurfaceFormat::DeprecatedFunctions);
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setAlphaBufferSize(0);
	format.setSamples(0);
	QSurfaceFormat::setDefaultFormat(format);

	pending_osgearth_app_dir = app_dir;
	ensure_osgearth_runtime_initialized_impl();
}

void ensure_osgearth_runtime_initialized_impl()
{
	if (osgearth_runtime_initialized) {
		return;
	}

	if (!pending_osgearth_app_dir.isEmpty()) {
		configure_osgearth_paths(pending_osgearth_app_dir, true);
	}

	osg::DisplaySettings::instance()->setGLContextVersion("3.3");
	osg::DisplaySettings::instance()->setGLContextProfileMask(0x00000002);
	osg::DisplaySettings::instance()->setShaderHint(osg::DisplaySettings::SHADER_GL2);
	osg::DisplaySettings::instance()->setMinimumNumStencilBits(1);

	osgEarth::initialize();

	shader_compatibility_host = new osg::Referenced;
	osgEarth::Registry::shaderFactory()->addPostProcessorCallback(
		shader_compatibility_host.get(), force_compatibility_glsl_header);
	osgearth_runtime_initialized = true;
}
}

void ensure_osgearth_runtime_initialized()
{
	ensure_osgearth_runtime_initialized_impl();
}

int main(int argc, char *argv[])
{
	const QString app_dir = app_dir_from_module(argc > 0 ? argv[0] : nullptr);
	prepare_osgearth_runtime(app_dir);
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(true);
	const QIcon appIcon(QStringLiteral(":/UiIcon/ICGAS_256.png"));
	app.setWindowIcon(appIcon);

	InfoExpert* expert = InfoExpert::GetInstance();
	if (expert->admin_mode) {
		UiICGAS::GetInstance()->setWindowIcon(appIcon);
		UiICGAS::GetInstance()->show();
		return app.exec();
	}

	else {
		UiLaunch window;
		window.setWindowIcon(appIcon);
		window.show();
		return app.exec();
	}
}
