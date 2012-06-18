#include <QtCore>
#include <iostream>
#include <unistd.h>

using namespace std;
QMap<QString, QString> master_data;
QString header ="Instance    ";
bool showheader=true;
QStringList args;

void showMetaData() {
	QString version = QString("$Revision: 1.7 $");
	version.replace("$","");
	version.replace("Revision:","");
	cout << "Author: Bill Anderson" <<endl;
	cout << "Author Email: <bill@baldguysoftware.com>" <<endl;
	cout << "Version: " <<version.toStdString() <<endl;
	cout << "Description: A tool for showing current Postfix queue levels. Supports multiple instances via config file" <<endl;
}

void showHelp() {
	QString usage = QString("Usage: %1 [--meta] [--help] [--config=/path/configfile.ini] [seconds]") .arg(args[0]);
	cout << usage.toStdString() <<endl;
}

/*
 * Used in showStats so placed here. But I don't want new copies every time
 */
QMap<QString,int> queuestats;
QString reportline;
QString instance;
QString error ;
QString queue;
QString fsqd;
QString qrrd;
QString result;
QString data;
QString key;
QString entry ;
QStringList queuedirs;
QString queue_root;
QString instance_name;
QStringList instances;

void showStats(QString cfile) {
	QSettings settings(cfile,QSettings::IniFormat);
	instances = settings.value("master/instances").toStringList();
	QStringListIterator i_instances(instances);
	bool have_header=false;
        int count = 0;


	while (i_instances.hasNext() ) {
                instance = i_instances.next();
		settings.beginGroup(instance);
		instance_name = settings.value("instance_name").toString();
		queue_root = settings.value("queue_root").toString();
		queuedirs = settings.value("queues").toStringList();
                settings.endGroup();
                reportline = QString("%1") .arg(instance_name,-12);

		QDir qroot(queue_root);
		if (!qroot.exists() ) {
                        error = QString("WARNING: Configured spool directory does not exist:%1 skipping %2").arg(queue_root).arg(instance_name);
                        qWarning( "WARNING: Configured spool directory does not exist:%s skipping %s", queue_root.toAscii().constData(), instance_name.toAscii().constData());
			//cout << error.toStdString() <<endl;
			continue;
                } else {
                   for (int i=0; i< queuedirs.size(); ++i) {
			QProcess finder;
                        queue = queuedirs.at(i);

                        fsqd = QString("%1/%2") .arg(queue_root) .arg( queue );
                        qrrd = QString("%1/%2_queue.rrd") .arg(queue_root).arg(queue);
			finder.start("find", QStringList() << fsqd << "-type" <<"f" );
			if (!finder.waitForStarted()) {
                            qFatal("unable to locate 'find' command, aborting");
			}
			if (!finder.waitForFinished()) {
                            qFatal("unable to locate 'find' command, aborting");
			}
                        result = finder.readAll();
                        count = result.split("\n").size()-1;
                        data= QString("N:%1") .arg(count);
			queuestats[queue] = count;
                    }
                    QMapIterator<QString,int> i_statmap(queuestats);
                    QStringListIterator i_keys(queuestats.keys());
                    while ( i_keys.hasNext() ) {
                        key = i_keys.next();
			if (! have_header ) {
                            header.append(QString("%1")	.arg(key,9));
			}
                    }
                    have_header=true;

                    // Now for the values
                    entry ="";
                    while (i_statmap.hasNext() ) {
			i_statmap.next();
			entry.append( QString("%1") .arg(i_statmap.value(),9) ); 
                    }
                    reportline.append(entry);
                    reportline.append("\n");
                    master_data[instance_name]=reportline;

                    QMapIterator<QString,QString> i_master_data(master_data);
                    if (showheader) {
                        cout << header.toStdString() <<endl;
                        cout << "======================================================================" <<endl;
                        showheader=false;
                    }
                    while (i_master_data.hasNext()) {
                        i_master_data.next();
                        cout << i_master_data.value().toStdString();
                    }
                }
        }
}

int main(int argc,char* argv[]) {
	QString cfile="";
	int interval=0;

	int acntr = 0;
	while (acntr < argc) {
		args << argv[acntr];
		acntr++;
	}
	
	if ( args.contains("--meta") ) {
		showMetaData();
		return 0;
	} else if (args.contains("--help")) {
		showHelp();
		return 0;
	}

	if ( args.size() ==2 ) {
		QString option = args[1];
		if ( option.startsWith("--config") ) {
			QString configopt = args[1].split("=")[1];
			cfile.append(configopt);
		} else {
			cfile.append("/etc/qshowpost.ini");
			interval = args[1].toInt();
		}
	} else {
		cfile.append("/etc/qshowpost.ini");
	}
	QFileInfo cf(cfile);
	if (! cf.exists() ) {
		QString error = QString("Unable to open %1, does not exist") .arg(cfile);
                qFatal( "Unable to open spool directory %s, it does not exist", cfile.toAscii().constData() );
	}

	if (interval==0) {
		showStats(cfile);
	} else {
		cout << "running in constant mode, CTRL-C to exit" <<endl;
		while (true) {
			showStats(cfile);
			sleep(interval);
		}
	}

	return 0;

}
