#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QButtonGroup>

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	void updateInfo(int r);
	void movePage(int p);
	QString genWhere();
	void updateThumb(const QByteArray& data);

private:
	Ui::MainWindow *ui;
	int curpage;
	int maxpage;
	QSqlDatabase database;
	QSqlQuery query;
	QNetworkAccessManager manager;
	QNetworkReply* reply;
	QString message;
	QString order;
	QString by;
	QButtonGroup* ordergroup;
	QButtonGroup* bygroup;
};

#endif // MAINWINDOW_H
