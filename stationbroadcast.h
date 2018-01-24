#ifndef STATIONBROADCAST_H
#define STATIONBROADCAST_H

#include <QObject>
#include<QThread>
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>
#include<QFile>
#include"qdebug.h"
#include <vector>
#include<qstring.h>
#include<qprocess.h>
#include "tools.h"
#include"Worker.h"

using namespace std;

/**
 * @brief The Station class 单个站点信息
 */
class Station{

public :
    QString name;
    int  index = -1;
    int type = -1;
    double longitude;
    double latitude;
    int stationNumber;
    int limitSpeed;
    QString textMsg1;
    QString textMsg2;
    QString direction;
    vector<QString> inVoices;
    vector<QString> outVoices;

    Station() {

    }
};

/**
 * @brief The StationBroadcast class
 * 报站逻辑说明：
 * 使用容量为4的站点队列，只要匹配到符合入站条件的站点均压入队列。初始情况下，若队列只有一个站点
 * 并且是上行或者下行站点集合的首部，或者队列内部任意两个站点方向相同且有序连续，则认为正常。为了
 * 去除队列中的无效数据，当队列两个连续的站点方向相同且序列逆序则剔除两个点,且正常进站后清空队列。
 */
class StationBroadcast : public QObject
{
    Q_OBJECT

public:
    typedef enum {
        IN_STATION,//进站状态
        ON_ROAD//站点之间的路上
    }STATE_IN_OUT_STATION;

    typedef enum{
        INVOICE,
        OUTVOICE
    }VOICE_TYPE;

    explicit StationBroadcast(QObject *parent = 0);
    ~StationBroadcast();

    //主函数
    void func(double lat,double lng);

signals:
    void playVoice(const QStringList&);

private:
    STATE_IN_OUT_STATION mInOutState = ON_ROAD;
    double mInStationDistance;//进站距离
    double mOutStationDistance;//出站距离
    vector<Station> mUpgoingStations;
    vector<Station> mDowngoningStations;
    vector<Station> mStationQueue;
    vector<Station>::iterator mStationIter;
    Station mCurStation;
    Station mLastStation;

    Worker *mWorker;
    QThread* mThread;

    //辅助函数
    void playVoices(Station station,VOICE_TYPE type);
    void loadStationsFromFile();

    //工具函数
    void pushStation(Station station);
    void removeWastedStationFromQueue();
    vector<Station> matchGoInStation(double lat,double lng);
    bool judgeIfOutStation(Station station,double lat,double lng);
    bool hasStartStationInQueue();
    bool judgeIfQueueNormalContinue();
    bool hasNormalContinueLastStationWithQueue();
};

#endif // STATIONBROADCAST_H
