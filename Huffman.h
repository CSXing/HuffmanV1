/*****************************************************************************
创建: 『宅』，QQ: 910955951
时间: 2019-01-01 09:00:00
描述: 哈夫曼压缩、解压源码，支持VC6以上版本
*****************************************************************************/
#ifndef __Huffmans_Hd
#define __Huffmans_Hd

#include <windows.h>

#include <cstdlib>
#include <queue>

using namespace std;

const int HM_TbCount=257, HM_IdCount=192, HM_RegCount=6;

/* ==================================
    CHuffman.Tables<哈夫曼.树/表>
   ================================== */
typedef struct Hm_Headers
{
    unsigned char _Heads[5];        //  5字节，zHAi
    unsigned char _Version;        //  1字节，版本号 0-255
} *Hm_pHeaders;

// 数据块定义
typedef struct Hm_DataBlocks
{
    char Path[256]; // 路径
    long Offset;    // 位置
    char *Datas;    // 数据
    long Length;    // 大小
    void Create(long _Size){
        Offset = 0;
        Datas  = (char*)malloc(sizeof(char)*_Size);
        Length = _Size;
    }
    // 获取
    unsigned char Get( long _Offset = -1 ){
        if (_Offset==-1)
            return Datas[Offset++];
        else
            return Datas[_Offset];
    }
    // 设置
    void Set(char _Byte, long _Offset = -1 ){
        if (_Offset==-1)
            Datas[Offset++]=_Byte;
        else
            Datas[_Offset]=_Byte;
    }
} *Hm_pDataBlocks;

typedef struct Hm_Tables
{
    int Id;
    unsigned int Count; // 出现次数统计
    char   Binary[64];  // 二进制字符串
    size_t Length;      // 2进制字符串占位长度，从左往右读取 如占4位: '01000000'='0100'
    Hm_Tables *left, *right, *parent;
    // 初始化
    void InitDef(){
        Id        = -1;
        Count     = 0;
        Binary[0] = '\0';
        Length    = 0;
        left      = NULL;
        right     = NULL;
        parent    = NULL;
    }
} *Hm_pTables;

/* ==================================
      CHuffman<哈夫曼>压缩/解压类
   ================================== */
class CHuffman
{
private:
    Hm_pHeaders    _Heads; // 文件头
    Hm_pDataBlocks _inDatas; // 输入数据
    Hm_pDataBlocks _ouDatas; // 输出数据
    bool isZIPs;
    //映射表[叶子节点数组]
    Hm_pTables _Root;
    Hm_pTables _HmTbs[HM_TbCount];
// 压缩定义
    // 从<哈夫曼>树.映射表 => 优先队列 => <哈夫曼>树
    void Buids();
        // 创建<哈夫曼>树.映射表
        void BuidTables(const Hm_pTables, bool);
    // 开始压缩过程
    void do_ZIP();
// 解压定义
    // 从<哈夫曼>映射表 => 优先队列 => 重建<哈夫曼>树
    BOOL reBuids();
    // 开始解压过程
    void do_UnZIP();
// 输出到文件
    void do_toFile();
    // 用于比较优先队列中元素间的顺序
    class Compare
    {
    public:
        bool operator()(const Hm_pTables& tbMax, const Hm_pTables& tbMin) const
        {
            return (*tbMax).Count > (*tbMin).Count;
        }
    };
    priority_queue< Hm_pTables, vector<Hm_pTables>, Compare > pq;

    // 初始化
    void Init(char *inPath, char *outPath);
public:
	char *Datas(){ return _ouDatas->Datas;}
	long Length(){ return _ouDatas->Length;}

    // 初始化对象()
    CHuffman(){
        _Heads = new Hm_Headers();
    };
    // 析构函数
    ~CHuffman(){
        delete _Heads;
        delete _inDatas;
        delete _ouDatas;
    };

    // 压缩并加密文件
    BOOL Encrypt(char *, char *);
    // 解密并解压文件
    BOOL Decrypt(char *, char *);
};
#endif
