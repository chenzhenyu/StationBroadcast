# StationBroadcast
公交报站器逻辑核心代码，满足绝大多数线路

逻辑：
    使用容量为4的站点队列，只要匹配到符合入站条件的站点均压入队列。初始情况下，若队列只有一个站点并且是上行或者下行站点集合的首部，或者队列内部任意两个站点方向相同且有序连续，则认为正常。为了去除队列中的无效数据，当队列两个连续的站点方向相同且序列逆序则剔除两个点,且正常进站后清空队列
    
使用：
    此项目为公司项目，涉及保密协议，为公开所有代码。使用时，将这些文件拷贝到自己的项目中，去除stationbroadcast.h .c文件中关于playVoice的部分（语音播报自行实现）。在GPS位置信号回调函数中执行func函数即可。
