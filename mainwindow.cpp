#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSqlError>
#include <QRegularExpression>
#include <QDateTime>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QBuffer>
#include <QEventLoop>
#include <QDebug>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>

static QSettings* setting;

static bool thumb_enable;
static QString thumb_prefix;
static QString thumb_suffix;
const char thumb_aes_key[17] = "awslawslawslawsl";

//const QString dbpath = R"(C:\Users\liaoh\Documents\EHentaiSearch\eh.db)";
static QString dbpath;

QMap<QString, QVector<QString>> parseTag(QString tags) {
	QMap<QString, QSet<QString>> result;
	QRegularExpression re(R"!!!( *([^: ]+):(?:([^ ]+)|"([^"]+)$") *)!!!");
	int offset = 0;
	QRegularExpressionMatch match;
	while (true) {
		match = re.match(tags, offset);
		if (!match.hasMatch()) {
			break;
		}
		result[match.captured(1)].insert(match.captured(2));
		offset = match.capturedEnd(0);
	}
	QMap<QString, QVector<QString>> ret;
	for (const QString& k: result.keys()) {
		ret[k].resize(result[k].size());
		std::copy(result[k].begin(), result[k].end(), ret[k].begin());
		std::sort(ret[k].begin(), ret[k].end());
	}
	return ret;
}

static QString bys[2] = {
	"posted",
	"rating"
};

static QString orders[2] = {
	" desc",
	"",
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), reply(nullptr) {
	ui->setupUi(this);
	ordergroup = new QButtonGroup(this);
	ordergroup->addButton(ui->desc, 0);
	ordergroup->addButton(ui->asc, 1);
	ui->desc->setChecked(true);
	bygroup = new QButtonGroup(this);
	bygroup->addButton(ui->byposted, 0);
	bygroup->addButton(ui->byrating, 1);
	ui->byposted->setChecked(true);
	by = bys[0];
	order = orders[0];
	setting = new QSettings("EHentaiSearch.ini", QSettings::IniFormat);
	dbpath = setting->value("database/path", "eh.db").toString();
	thumb_enable = setting->value("thumb/enable", false).toBool();
	thumb_prefix = setting->value("thumb/prefix", "").toString();
	thumb_suffix = setting->value("thumb/suffix", "").toString();
	database = QSqlDatabase::addDatabase("QSQLITE");
	database.setDatabaseName(dbpath);
	if (!database.open()) {
		QMessageBox::critical(this, "EHentaiSearch", "无法打开数据库");
		exit(0);
	}
	QLabel* update = new QLabel("构建于 " __DATE__ " " __TIME__ " <a href='http://github.com/ex-rip/EHentaiSearch'>主页</a>", ui->status);
	update->setOpenExternalLinks(true);
	ui->status->addPermanentWidget(update);
	query = QSqlQuery(database);
	connect(bygroup, QOverload<int>::of(&QButtonGroup::buttonClicked), [&](int id) {
		by = bys[id];
	});
	connect(ordergroup, QOverload<int>::of(&QButtonGroup::buttonClicked), [&](int id) {
		order = orders[id];
	});
	connect(ui->prev, &QPushButton::clicked, [&]() {
		int n = curpage;
		--n;
		if (n == 1) {
			ui->prev->setEnabled(false);
		}
		if (n < maxpage) {
			ui->next->setEnabled(true);
		}
		ui->page->setValue(n);
		movePage(n);
	});
	connect(ui->next, &QPushButton::clicked, [&]() {
		int n = curpage;
		++n;
		if (n == maxpage) {
			ui->next->setEnabled(false);
		}
		if (n > 1) {
			ui->prev->setEnabled(true);
		}
		ui->page->setValue(n);
		movePage(n);
	});
	connect(ui->search, &QPushButton::clicked, [&]() {
		disconnect(ui->result);
		disconnect(ui->page);
		ui->result->clear();
		ui->status->showMessage("搜索中，请稍候。");
		ui->status->repaint();
		if (!query.exec("select count(id) from gallery " + genWhere() + ";")) {
			QMessageBox::critical(this, "EX R.I.P.", "搜索失败:" + query.lastError().text());
			exit(0);
		}
		query.next();
		int results = query.value(0).toInt();
		ui->status->showMessage("搜索出结果" + QString::asprintf("%d", results) + "条。搜索中，请稍候。");
		ui->status->repaint();
		if (results) {
			ui->page->setEnabled(results > 20);
			maxpage = (results - 1) / 20 + 1;
			curpage = 1;
			ui->page->setRange(1, (results - 1) / 20 + 1);
			ui->page->setValue(1);
			ui->prev->setEnabled(false);
			ui->next->setEnabled(maxpage > 1);
			connect(ui->page, &QSpinBox::editingFinished, [&]() {
				this->movePage(ui->page->value());
			});
			if (!query.exec("select * from gallery " + genWhere() + " order by " + by + order + " limit 20;")) {
				QMessageBox::critical(this, "EX R.I.P.", "搜索失败:" + query.lastError().text());
				exit(0);
			}
			while (query.next()) {
				QString title;
				if (query.value(3).toString().length() > 0) {
					title = query.value(3).toString();
				} else {
					title = query.value(2).toString();
				}
				ui->result->addItem(title);
			}
			connect(ui->result, &QListWidget::currentRowChanged, this, &MainWindow::updateInfo);
		} else {
			maxpage = 1;
			curpage = 1;
			ui->page->setRange(1, 1);
			ui->page->setValue(1);
			ui->page->setEnabled(false);
			ui->prev->setEnabled(false);
			ui->next->setEnabled(false);
		}
		updateInfo(-1);
		message = QString::asprintf("结果共计%d条，%d页。", results, maxpage);
		ui->status->showMessage(message);
	});
}

MainWindow::~MainWindow() {
	delete ui;
}

QString MainWindow::genWhere() {
	auto tags = parseTag(ui->tag->text()
						 .replace("/", "//")
						 .replace("'", "''")
						 .replace("[", "/[")
						 .replace("]", "/]")
						 .replace("%", "/%")
						 .replace("&", "/&")
						 .replace("_", "/_")
						 .replace("(", "/(")
						 .replace(")", "/)"));
	QList<QString> likes;
	for (const QString& k: {"artist", "group", "parody", "language", "character", "female", "male"}) {
		if (tags[k].size()) {
			QString rst = "%";
			for (const QString& v: tags[k]) {
				rst += v + "%";
			}
			likes.push_back(k + " like '" + rst + "'");
		}
	}
	if (tags["misc"].size()) {
		QString rst = "%";
		for (const QString& v: tags["misc"]) {
			rst += v + "%";
		}
		likes.push_back("rest like '" + rst + "'");
	}
	if (tags["category"].size()) {
		QList<QString> categories;
		for (QString category: tags["category"]) {
			category[0] = category[0].toUpper();
			categories.push_back("category = '" + category + "'");
		}
		likes.push_back("( " + categories.join(" or ") + " )");
	}
	if (tags["rating"].size()) {
		QList<QString> ratingreq;
		for (QString rating: tags["rating"]) {
			ratingreq.push_back("rating " + rating);
		}
		likes.push_back("( " + ratingreq.join(" and ") + " )");
	}
	if (tags["id"].size()) {
		QList<QString> idreq;
		for (QString id: tags["id"]) {
			idreq.push_back("id " + id);
		}
		likes.push_back("( " + idreq.join(" and ") + " )");
	}
	QString tag = likes.join(" and ");
	QString tit = "";
	if (ui->query->text().length()) {
		QString s = ui->query->text()
					.replace("/", "//")
					.replace("'", "''")
					.replace("[", "/[")
					.replace("]", "/]")
					.replace("%", "/%")
					.replace("&", "/&")
					.replace("_", "/_")
					.replace("(", "/(")
					.replace(")", "/)");
		tit = "title like '%" + s + "%' or title_jpn like '%" + s + "%'";
	}
	return tag.length() ? (tit.length() ? " where ( " + tit + " ) and ( " + tag + " )" : " where " + tag) : (tit.length() ? " where " + tit : "");
}

QByteArray decryption(QByteArray data) {
	int len = data.length();
	int blocks = len >> 4;
	unsigned char* ptr = reinterpret_cast<unsigned char*>(data.data());
	CryptoPP::AES::Decryption aesDecryption(reinterpret_cast<const unsigned char*>(thumb_aes_key), 16);
	while (blocks--) {
		aesDecryption.ProcessBlock(ptr);
		ptr += 16;
	}
	data.chop(*--ptr);
	return data;
}

void MainWindow::updateInfo(int r) {
	auto div = [](const QString& raw) -> QList<QString> {
		if (raw.length() == 0) {
			return QList<QString>();
		} else {
			auto rst = raw.mid(2, raw.length() - 4).split("', '");
			std::sort(rst.begin(), rst.end());
			return std::move(rst);
		}
	};
	if (reply) {
		disconnect(reply);
		reply->abort();
		reply->deleteLater();
		reply = nullptr;
	}
	if (r == -1) {
		ui->id->setText("0");
		ui->title->setText("EX R.I.P.");
		ui->titlejp->setText("EX R.I.P.");
		ui->category->setText("Misc");
		ui->uploader->setText("nekosu");
		ui->posted->setText("2019/7/26");
		ui->rating->setText("5.00");
		ui->artist->setText("Tenboro");
		ui->group->setText("Sad Panda");
		ui->parody->setText("");
		ui->language->setText("chinese translated");
		ui->misc->setText("");
		ui->female->clear();
		ui->male->clear();
		ui->character->clear();
		ui->urlF->setText("<a href=\"https://e-hentai.org/\">e-hentai</a>");
		ui->urlB->setText("<a href=\"https://exhentai.org/\">exhentai</a>");
		QPixmap pix(":/ex.jpg");
		ui->thumb->setPixmap(pix);
	} else {
		query.seek(r);
		ui->id->setText(query.value(0).toString());
		ui->title->setText(query.value(2).toString());
		ui->title->setCursorPosition(0);
		ui->titlejp->setText(query.value(3).toString());
		ui->titlejp->setCursorPosition(0);
		ui->category->setText(query.value(4).toString());
		ui->uploader->setText(query.value(5).toString());
		QDate tm = QDateTime().fromTime_t(query.value(6).toString().toULong()).date();
		ui->posted->setText(QString::asprintf("%d/%d/%d", tm.year(), tm.month(), tm.day()));
		ui->rating->setText(query.value(14).toString());
		ui->artist->setText(div(query.value(15).toString()).join(", "));
		ui->group->setText(div(query.value(16).toString()).join(", "));
		ui->parody->setText(div(query.value(17).toString()).join(", "));
		QList<QString> lang = div(query.value(21).toString());
		if (lang.length() == 0) {
			ui->language->setText("unknown");
		} else if (lang.length() == 1) {
			ui->language->setText(lang[0]);
		} else {
			ui->language->setText(lang[lang[0] == "translated"] + " translated");
		}
		ui->misc->setText(div(query.value(23).toString()).join(", "));
		ui->character->clear();
		ui->character->addItems(div(query.value(18).toString()));
		ui->female->clear();
		ui->female->addItems(div(query.value(19).toString()));
		ui->male->clear();
		ui->male->addItems(div(query.value(20).toString()));
		ui->urlF->setText("<a href=\"" + query.value(1).toString() + "\">e-hentai</a>");
		ui->urlB->setText("<a href=\"" + query.value(24).toString() + "\">exhentai</a>");
		if (thumb_enable) {
			int id = query.value(0).toString().toInt();
			QString cache = QString("cache") + QDir::separator() + QString::asprintf("%d", id);
			if (!QFile::exists(cache)) {
				int gid = (id - 1) / 32 + 1;
				QString url = thumb_prefix + QString::asprintf("%d", gid) + thumb_suffix;
				reply = manager.get(QNetworkRequest(QUrl(url)));
				ui->status->showMessage(message + "正在下载封面。");
				ui->status->repaint();
				connect(reply, &QNetworkReply::finished, [&]() {
					QByteArray data = reply->readAll();
					disconnect(reply);
					reply->deleteLater();
					reply = nullptr;
					QString cache = QString("cache") + QDir::separator() + QString::asprintf("%d", query.value(0).toString().toInt());
					QFile file(cache);
					file.open(QIODevice::WriteOnly);
					file.write(data);
					file.close();
					updateThumb(data);
					ui->status->showMessage(message);
					ui->status->repaint();
				});
			} else {
				QFile file(cache);
				file.open(QIODevice::ReadOnly);
				updateThumb(file.readAll());
				file.close();
			}
		}
	}
}

void MainWindow::updateThumb(const QByteArray& data) {
	QPixmap pixmap;
	pixmap.loadFromData(decryption(data));
	int w = pixmap.width(), h = pixmap.height();
	if (w >= h && w > 400) {
		pixmap.scaledToWidth(400);
	} else if (h > w && h > 400) {
		pixmap.scaledToHeight(400);
	}
	ui->thumb->setPixmap(pixmap);
}

void MainWindow::movePage(int p) {
	curpage = p;
	disconnect(ui->result);
	ui->result->clear();
	if (!query.exec("select * from gallery " + genWhere() + " order by " + by + order + " limit 20 offset " + QString::asprintf("%d", (p - 1) * 20) + ";")) {
		QMessageBox::critical(this, "EX R.I.P.", "搜索失败:" + query.lastError().text());
		exit(0);
	}
	while (query.next()) {
		QString title;
		if (query.value(3).toString().length() > 0) {
			title = query.value(3).toString();
		} else {
			title = query.value(2).toString();
		}
		ui->result->addItem(title);
	}
	connect(ui->result, &QListWidget::currentRowChanged, this, &MainWindow::updateInfo);
	updateInfo(-1);
}
