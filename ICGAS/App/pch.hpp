#ifndef PCH_HPP
#define PCH_HPP


// UI中文显示
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

// ---------- C/C++标准库头文件 ----------
#include <numbers>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <variant>
#include <utility>
#include <algorithm>
#include <cmath>

// ---------- 通信相关头文件 ----------
#include <QHostAddress>	// IP地址
#include <QUdpSocket>	// UDP通信
#include <QTcpServer>	// TCP服务端
#include <QTcpSocket>	// TCP客户端
#include <QtEndian>		// 大小端转换



// ---------- Qt相关头文件 ----------
#include <QtWidgets/QMainWindow>
#include <QApplication>	// Qt应用对象
#include <QMutex>		// 线程同步
#include <QTimer>		// 定时器
#include <QObject>		// Qt对象基类
#include <QThread>		// Qt线程
#include <QPointer>		// QObject安全指针
#include <QScreen>		// 屏幕控制
#include <QDebug>		// 调试输出

#include <QSharedPointer>	// 智能指针
#include <QMetaType>		// Qt元类型

#include <QDateTime>	// 日期时间
#include <QRegularExpression>	// 正则表达式
#include <QtGlobal>		// Qt全局定义
#include <QString>		// 字符串类型
#include <QStringList>	// 字符串列表
#include <QList>		// 列表类型
#include <QDir>			// 目录操作
#include <QFile>		// 文件操作
#include <QFileInfo>	// 文件信息
#include <QJsonArray>	// 处理JSON数组
#include <QJsonDocument>	// 处理JSON文档
#include <QJsonObject>	// 处理JSON对象
#include <QJsonParseError>	// JSON解析错误
#include <QJsonValue>	// JSON数据值
#include <QTextStream>	// 文本流操作
#include <QStringConverter>	// 字符串编码转换
#include <QVariantList>	// QML列表数据
#include <QVariantMap>	// QML键值数据
#include <QThreadPool>	// 后台任务

// ---------- GUI相关头文件 ----------
#include <QGuiApplication>		// 获取屏幕信息
#include <QMetaObject>			// Qt元对象系统，跨线程调用
#include <QIcon>				// 图标控制
#include <QTableWidget>			// 表格控件
#include <QAbstractItemView>	// 表格视图控制
#include <QTableWidgetItem>		// 表格单元格项
#include <QHeaderView>			// 表头控制
#include <QScrollBar>			// 滚动条控制
#include <QSignalBlocker>		// 信号阻塞器，避免更新表格时触发选中等信号
#include <QQmlApplicationEngine>	// QML应用引擎
#include <QQmlContext>			// QML上下文
#include <QQuickWindow>			// QML窗口
#include <QMessageBox>			// 提示框


#endif // !PCH_HPP
