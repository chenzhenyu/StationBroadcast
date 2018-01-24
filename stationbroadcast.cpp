#include "stationbroadcast.h"

StationBroadcast::StationBroadcast(QObject *parent) : QObject(parent) {
    //初始化播放模块
    mWorker=new Worker();
    mThread =new QThread();
    mWorker->moveToThread(mThread);
    connect(this,SIGNAL(playVoices(QStringList)),mWorker,SLOT(PlayVoice(QStringList)));
    mThread->start();

    //加载数据
    loadStationsFromFile();
    //    for(int i = 0;i<mUpgoingStations.size();i++) {
    //        qDebug()<<mUpgoingStations[i].index;
    //    }
}

StationBroadcast::~StationBroadcast(){

}

void StationBroadcast::func(double lat,double lng) {
    vector<Station> goInStations =  matchGoInStation(lat,lng);
    for(int i = 0;i < goInStations.size();i++) {
        pushStation(goInStations[i]);
        removeWastedStationFromQueue();
    }

    //    for(int i = 0;i < mStationQueue.size();i++) {
    //        qDebug() << "Queue:"<<mStationQueue[i].type<<"|"<<mStationQueue[i].index;
    //    }

    //车辆在起始站点内启动的情况
    if(mInOutState == ON_ROAD && hasStartStationInQueue() && mLastStation.index == -1) {
        mInOutState = IN_STATION;
        for(int i = 0;i<mStationQueue.size();i++) {
            if(mStationQueue[i].index == 1) {
                mCurStation = mStationQueue[i];
            }
        }
        playVoices(mCurStation,INVOICE);
        mStationQueue.clear();
        return;
    }

    //车辆出站
    if(mInOutState == IN_STATION &&  judgeIfOutStation(mCurStation,lat,lng)) {
        playVoices(mCurStation,OUTVOICE);

        mInOutState = ON_ROAD;
        mLastStation = mCurStation;
        mCurStation = Station();
        mStationQueue.clear();
        return;
    }

    //运行过程中进站(与mLastStation连续)
    if(mInOutState == ON_ROAD && hasNormalContinueLastStationWithQueue()) {
        for(int i =0;i<mStationQueue.size();i++) {
            if(mStationQueue[i].type == mLastStation.type &&
                    mStationQueue[i].index == mLastStation.index+1){
                mCurStation = mStationQueue[i];
                mInOutState = IN_STATION;
                playVoices(mCurStation,INVOICE);
                mStationQueue.clear();
                return;
            }
        }
    }

    //运行过程中进站(队列中有连续站点，主要针对纠错情况)
    if(mInOutState == ON_ROAD && judgeIfQueueNormalContinue()) {
        for(int i = 1;i < mStationQueue.size();i++) {
            for(int n = 0;n<i;n++) {
                if(mStationQueue[n].type == mStationQueue[i].type &&
                        mStationQueue[i].index - mStationQueue[n].index == 1) {
                    mCurStation = mStationQueue[i];
                    mInOutState = IN_STATION;
                    playVoices(mCurStation,INVOICE);
                    mStationQueue.clear();
                    return;
                }
            }
        }
    }
    //    qDebug()<<"InOutState:" << mInOutState;
    //    qDebug()<<"CurStation:"<<mCurStation.type<<"|"<<mCurStation.index;
    //    qDebug()<<"lastStation:"<<mLastStation.type<<"|"<<mLastStation.index;
}

void StationBroadcast::playVoices(Station station,VOICE_TYPE type) {
    QStringList playlist;

    if(type == INVOICE){
        if(station.inVoices.size()>0) {
            for(int i=0;i<station.inVoices.size();i++) {
                playlist<<"wav/"+station.inVoices[i];
            }
        }
    }else{
        if(station.outVoices.size()>0) {
            for(int i=0;i<station.outVoices.size();i++) {
                playlist<<"wav/"+station.outVoices[i];
            }
        }
    }

    if(!playlist.isEmpty())
    {
        mWorker->minusStop();
        mWorker->PlayVoice(playlist);
        //emit playVoice(playlist);
    }
}

void StationBroadcast::loadStationsFromFile() {
    QFile file;
    file.setFileName("lineinfo.json");
    if(!file.open(QIODevice::ReadOnly))
    {
        return;
    }
    QByteArray data =file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data,&error);
    QJsonObject line = doc.array().at(0).toObject();
    QJsonObject judgeDistance = line.value("judgeDistance").toObject();
    mInStationDistance = judgeDistance.value("inStationDistance").toDouble()/(double)1000;
    mOutStationDistance = judgeDistance.value("outStationDistance").toDouble()/(double)1000;
    QJsonArray up = line.value("upgoingStations").toArray();
    QJsonArray down = line.value("downgoningStations").toArray();
    for(int i = 0;i < up.size();i++) {//上行
        Station station;
        QJsonObject s = up.at(i).toObject();
        station.name = s.value("Name").toString();
        station.index = s.value("Index").toInt();
        station.type = s.value("Type").toInt();
        station.longitude = s.value("Longitude").toDouble();
        station.latitude = s.value("Latitude").toDouble();
        station.stationNumber = s.value("StationNumber").toInt();
        station.limitSpeed= s.value("LimitSpeed").toInt();
        station.textMsg1 = s.value("TextMsg1").toString();
        station.textMsg2 = s.value("TextMsg2").toString();
        station.direction = s.value("Direction").toString();

        QJsonArray inVoices = s.value("InVoices").toArray();
        for(int n = 0;n<inVoices.size();n++) {
            QJsonObject voice = inVoices.at(n).toObject();
            station.inVoices.push_back(voice.value("Name").toString());
        }

        QJsonArray outVoices = s.value("OutVoices").toArray();
        for(int n = 0;n<outVoices.size();n++) {
            QJsonObject voice = outVoices.at(n).toObject();
            station.outVoices.push_back(voice.value("Name").toString());
        }

        mUpgoingStations.push_back(station);
    }

    for(int i = 0;i < down.size();i++) {//下行
        Station station;
        QJsonObject s = down.at(i).toObject();
        station.name = s.value("Name").toString();
        station.index = s.value("Index").toInt();
        station.type = s.value("Type").toInt();
        station.longitude = s.value("Longitude").toDouble();
        station.latitude = s.value("Latitude").toDouble();
        station.stationNumber = s.value("StationNumber").toInt();
        station.limitSpeed= s.value("LimitSpeed").toInt();
        station.textMsg1 = s.value("TextMsg1").toString();
        station.textMsg2 = s.value("TextMsg2").toString();
        station.direction = s.value("Direction").toString();

        QJsonArray inVoices = s.value("InVoices").toArray();
        for(int n = 0;n<inVoices.size();n++) {
            QJsonObject voice = inVoices.at(n).toObject();
            station.inVoices.push_back(voice.value("Name").toString());
        }

        QJsonArray outVoices = s.value("OutVoices").toArray();
        for(int n = 0;n<outVoices.size();n++) {
            QJsonObject voice = outVoices.at(n).toObject();
            station.outVoices.push_back(voice.value("Name").toString());
        }

        mDowngoningStations.push_back(station);
    }
    file.close();
}

/**
 * @brief StationBroadcast::pushStation 将检测到入站的站点压入队列，保证压入站点不重复
 * @param station
 */
void StationBroadcast::pushStation(Station station) {
    if(mStationQueue.size() >=4) {
        mStationQueue.erase(mStationQueue.begin());
        pushStation(station);
    }else {
        for(int i = 0;i < mStationQueue.size();i++) {
            if(station.type == mStationQueue[i].type&&station.index == mStationQueue[i].index) {
                return;
            }
        }
        mStationQueue.push_back(station);
    }
}

void StationBroadcast::removeWastedStationFromQueue() {
    if(mStationQueue.size() < 2) return;
    for(int i = 1;i < mStationQueue.size();i++) {
        for(int n = 0;n<i;n++) {
            if(mStationQueue[n].type == mStationQueue[i].type &&
                    mStationQueue[n].index - mStationQueue[i].index > 0) {
                mStationQueue.erase(mStationQueue.begin()+i);
                mStationQueue.erase(mStationQueue.begin()+n);
                removeWastedStationFromQueue();
                return;
            }
        }
    }
}

vector<Station> StationBroadcast::matchGoInStation(double lat,double lng) {
    vector<Station> stations;
    for(int i = 0;i < mUpgoingStations.size();i++) {
        if(mInStationDistance >= distanceEarth(mUpgoingStations[i].latitude,mUpgoingStations[i].longitude,lat,lng)) {
            stations.push_back(mUpgoingStations[i]);
        }
    }
    for (int i = 0;i < mDowngoningStations.size(); i++) {
        if(mInStationDistance >= distanceEarth(mDowngoningStations[i].latitude,mDowngoningStations[i].longitude,lat,lng)) {
            stations.push_back(mDowngoningStations[i]);
        }
    }
    return stations;
}

bool StationBroadcast::judgeIfOutStation(Station station,double lat,double lng) {
    if(station.index == -1) return true;
    if(mOutStationDistance < distanceEarth(station.latitude,station.longitude,lat,lng)) {
        return true;
    }else {
        return false;
    }
}

/**
 * @brief StationBroadcast::hasStartStationInQueue 是否在队列中有上行或下行线路的起始站点
 * @return
 */
bool StationBroadcast::hasStartStationInQueue() {
    if(mStationQueue.size()==0)return false;
    for(int i = 0;i < mStationQueue.size();i++) {
        if(mStationQueue[i].index == 1) {
            return true;
        }
    }
    return false;
}

/**
 * @brief StationBroadcast::judgeIfQueueNormalContinue 判断队列中是否有符合车辆运行的连续站点
 * @return
 */
bool  StationBroadcast::judgeIfQueueNormalContinue() {
    if(mStationQueue.size() < 2) return false;
    for(int i = 1;i < mStationQueue.size();i++) {
        for(int n = 0;n<i;n++) {
            if(mStationQueue[n].type == mStationQueue[i].type &&
                    mStationQueue[i].index - mStationQueue[n].index == 1) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief StationBroadcast::hasNormalContinueLastStationWithQueue 队列是否中有和mLastStation正序连续的站点
 * @return
 */
bool StationBroadcast::hasNormalContinueLastStationWithQueue() {
    if(mLastStation.index == -1) return false;
    if(mStationQueue.size() <= 0) return false;
    for(int i =0;i<mStationQueue.size();i++) {
        if(mStationQueue[i].type == mLastStation.type &&
                mStationQueue[i].index == mLastStation.index+1){
            return true;
        }
    }
    return false;
}

