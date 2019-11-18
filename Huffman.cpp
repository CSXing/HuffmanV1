/*****************************************************************************
创建: 『宅』，QQ: 910955951
时间: 2019-01-01 09:00:00
描述: 哈夫曼压缩、解压源码，支持VC6以上版本
*****************************************************************************/
#include "Huffman.h"

/* ==================================
       构造优先队列、构造树、映射表
   ================================== */
void CHuffman::Buids()
{
    // 创建<哈夫曼>映射表 (unsigned char 无符号计算时不丢失)
    _inDatas->Offset=0;
    while( _inDatas->Offset<_inDatas->Length )
    {
        int nId = (int)_inDatas->Get();
        _HmTbs[nId]->Count++;
    }
    // 创建优先队列
    for( int _Id=0; _Id<HM_TbCount; _Id++ )
    {
        if ( _HmTbs[_Id]->Count>0 ){
            _HmTbs[_Id]->Id = (int)_Id;
            pq.push(_HmTbs[_Id]);
        }
    }
    /* ==创建<哈夫曼>树, 内部对象:<哈夫曼>表== */
    _Root = NULL;
    while( !pq.empty() )
    {       
        Hm_pTables first = pq.top();
        pq.pop();
        if( pq.empty() ) {
            _Root = first;
            break;
        }
        Hm_pTables second = pq.top();
        pq.pop();
        Hm_pTables newTable = new Hm_Tables();
        newTable->InitDef();
        newTable->Count = first->Count + second->Count;

        if(first->Count <= second->Count) {
            newTable->left = first;
            newTable->right = second;
        } else {
            newTable->left  = second;
            newTable->right = first;
        }
        first->parent  = newTable;
        second->parent = newTable;

        pq.push(newTable);
    }

    // 计算<哈夫曼>编码
    if(_Root->left != NULL)
        BuidTables(_Root->left, true);
    if(_Root->right != NULL)
        BuidTables(_Root->right, false);
}
/* ==================================
             计算<哈夫曼>编码
   ================================== */
void CHuffman::BuidTables(const Hm_pTables table, bool left)
{
    int i=0;
    for (i=0; i<table->parent->Length; i++)
        table->Binary[i] = table->parent->Binary[i];

    if (left)
        table->Binary[i++] = '0';
    else
        table->Binary[i++] = '1';

    table->Length = i;
    table->Binary[i] = '\0';

    // 是“有效”节点
    if(table->left == NULL && table->right == NULL)
        return;
    
    if(table->left != NULL)
        BuidTables(table->left, true);
    if(table->right != NULL)
        BuidTables(table->right, false);
}
/* ==================================
               压缩文件
   ================================== */
void CHuffman::do_ZIP()
{
    int i=0, j=0, iTb_LId=0, iTb_Len=0, iTb_bId=0, iTb_Bin=0, iBin_Len=0, iK=0, iKey_bId=0;
    unsigned char lenBit, binBit, movBit, inUChar, ouUChar;

    Buids();
    
// Bin数据
    _ouDatas->Create(_inDatas->Length+255*3);

// 原文件大小[3byte]峰值0xFFFFFF≈16Mb
    _ouDatas->Set((_inDatas->Length & 0x000000FF));
    _ouDatas->Set((_inDatas->Length & 0x0000FF00)>>8);
    _ouDatas->Set((_inDatas->Length & 0x00FF0000)>>16);

// 映射表 192byte
    iTb_LId = _ouDatas->Offset; // 二进制长度
    iTb_Len = 0;
    lenBit  = 0;
    iTb_bId = iTb_LId + HM_IdCount;    // 二进制数据
    iTb_Bin = 0;
    binBit  = 0;

    iKey_bId=iTb_LId;
    
    for(i=iTb_LId; i<iTb_bId; i++)
        _ouDatas->Set(0,i);
    
    for(i=0; i<HM_TbCount; i++){
        iBin_Len = 0;
        if ( _HmTbs[i]->Count > 0 ) iBin_Len = _HmTbs[i]->Length;

        // 二进制长度
        for( j=0; j<6; j++){
            lenBit += ((iBin_Len & 0x20>>j)>>(5-j))<<(7-iTb_Len);
            iTb_Len++;
            if ( iTb_Len>=8 ){
                _ouDatas->Set(lenBit,iTb_LId++);
                lenBit  = 0;
                iTb_Len = 0;
            }
        }

        if (iBin_Len>0){
            // 二进制数据
            for( j=0; j<iBin_Len; j++){
                movBit  = (int)(_HmTbs[i]->Binary[j]-'0');
                binBit += movBit<<(7-iTb_Bin);
                iTb_Bin++;
                if ( iTb_Bin>=8 ){
                    _ouDatas->Set(binBit,iTb_bId++);
                    binBit  = 0;
                    iTb_Bin = 0;
                }
            }
        }
    }
    if ( iTb_Bin>0) { _ouDatas->Set(binBit,iTb_bId);iTb_bId++; }
    _ouDatas->Offset = iTb_bId;
    iK = 0;
// 压缩文件 满8bit记录到1byte
    _inDatas->Offset = 0;
    inUChar = 0;
    ouUChar = 0;
    long _lastBit = 0;
    while( _inDatas->Offset<_inDatas->Length )
    {
        inUChar = (unsigned char)_inDatas->Get();
        char * _Binary = _HmTbs[inUChar]->Binary;
        while(*_Binary != '\0')
        {
            inUChar  = (int)(*_Binary-'0');
            ouUChar += inUChar<<(7-_lastBit);
            _lastBit++;

            if ( _lastBit>=8 ) {
                _ouDatas->Set(ouUChar);
                ouUChar  = 0; //初始化
                _lastBit = 0;
            }
            _Binary++;
        }
    }
// 压缩文件 补位
    if (_lastBit!=0) _ouDatas->Set(ouUChar);
    _ouDatas->Length = _ouDatas->Offset;
}
/* ==================================
从<哈夫曼>映射表 => 优先队列 => 重建<哈夫曼>树
   ================================== */
BOOL CHuffman::reBuids()
{
    int i=0, j=0, k=0, iTb_LId=0, iTb_Len=0, iTb_bId=0, iTb_Bin=0, iBin_Len=0, iK=0;
    unsigned char lenBit, binBit;
    Hm_pTables tmpNode, runNode, readNode;

    readNode = new Hm_Tables();
    readNode->InitDef();

    _Root = new Hm_Tables();
    _Root->InitDef();

    // 文件头
    _inDatas->Offset = 0; //sizeof(_Heads);
    for (i=0; i<sizeof(_Heads->_Heads); i++){
        if (_Heads->_Heads[i] != _inDatas->Get()) return (i+1);
    }
    if (_Heads->_Version != _inDatas->Get()) return (i+2);

// 原文件大小[3byte]峰值0xFFFFFF≈16Mb
    _ouDatas->Length  = _inDatas->Get();
    _ouDatas->Length += _inDatas->Get() * 0x100;
    _ouDatas->Length += _inDatas->Get() * 0x10000;

// 映射表 192byte
    iTb_LId  = 0; // 二进制长度
    iTb_Len  = 0;
    lenBit   = 0;
    iTb_bId  = _inDatas->Offset + HM_IdCount;
    iTb_Bin  = 0;
    binBit   = 0;
    iBin_Len = 0;

    iK = 0; int iPK=0;
    for(i=0; i<HM_IdCount; i++){
        lenBit = _inDatas->Get();
        // 二进制长度
        for( j=0; j<8; j++){
            iBin_Len += ((lenBit & 0x80>>j)>>(7-j))<<(5-iTb_Len);
            iTb_Len++;
            if (iTb_Len>=6){

                if (iBin_Len>0) {
                    readNode->Id     = iTb_LId;
                    readNode->Length = iBin_Len;

                    runNode = _Root;
                    binBit  = _inDatas->Get(iTb_bId);
                    for (k=0; k<iBin_Len; k++){
                        if (binBit & 0x80>>iTb_Bin){
                            readNode->Binary[k] = '1';
                            tmpNode = runNode->right;
                        } else {
                            readNode->Binary[k] = '0';
                            tmpNode = runNode->left;
                        }
                        // 创建Node
                        if(tmpNode == NULL){
                            // 记录压缩数据
                            if ( (k+1)==readNode->Length ){
                                tmpNode = _HmTbs[readNode->Id];
                                tmpNode->Id     = readNode->Id;
                                tmpNode->Count  = k+1;
                                tmpNode->parent = runNode;
                            } else {
                                tmpNode = new Hm_Tables();
                                tmpNode->InitDef();
                                tmpNode->parent = runNode;
                            }
                            int iCpy;
                            for (iCpy=0; iCpy<readNode->Length; iCpy++ )
                                tmpNode->Binary[iCpy] = readNode->Binary[iCpy];
                            tmpNode->Binary[iCpy] = '\0';
                            tmpNode->Length       = readNode->Length;

                            if ( binBit & 0x80>>iTb_Bin ){
                                runNode->right = tmpNode;
                            } else {
                                runNode->left = tmpNode;
                            }
                        } else if( (k+1)==readNode->Length ){
                            _ouDatas->Length = 0;
                            return 1;//节点存在
                        } else if( tmpNode->left == NULL && tmpNode->right == NULL ){
                            _ouDatas->Length = 0;
                            return 2; //寻路异常
                        }
                        runNode = tmpNode;

                        iTb_Bin++;
                        if ( iTb_Bin>=8 ){
                            iTb_bId++;
                            binBit  = _inDatas->Get(iTb_bId);
                            iTb_Bin = 0;
                        }
                    }
                }
                iTb_LId++;
                iBin_Len = 0;
                iTb_Len  = 0;
            }
        }
    }
    if ( iTb_Bin>0 ) iTb_bId++;
    _inDatas->Offset = iTb_bId;
    return FALSE;
}
/* ==================================
                解压数组
   ================================== */
void CHuffman::do_UnZIP()
{
    if (reBuids()!=FALSE){
        _ouDatas->Length = 0;
        return;
    }
    int readEnd=0, iK=0;
    unsigned char movBit, ouChar;
    Hm_pTables runNode;

    readEnd = 0;
// 压缩实体数据
    _ouDatas->Create(_ouDatas->Length); //Bin数据
    runNode = _Root;
    while( _ouDatas->Offset<_ouDatas->Length )
    {
        ouChar = _inDatas->Get();
        if (iK>=HM_RegCount) iK = 0;
        movBit = 0x80;
        while( movBit>0 )
        {
            if (ouChar & movBit){
                runNode = runNode->right;
            } else {
                runNode = runNode->left;
            }

            //if(runNode->left == NULL && runNode->right == NULL)
            if(runNode->Id != -1 )
            {
                _ouDatas->Set(runNode->Id);
                runNode = _Root;
                if (_ouDatas->Offset==_ouDatas->Length){
                    readEnd = 1;
                    break;
                }
            }
    
            movBit = movBit >> 1;
        }
        if (readEnd!=0) break;
    }
}
/* ==================================
                输出到文件
   ================================== */
void CHuffman::do_toFile()
{
    if ( _ouDatas->Length<=0 ) return;
    if ( sizeof(_ouDatas->Path)<=0 ) return;
// 释放资源
    free(_inDatas->Datas);
// 打开文件
    FILE *hFiles= fopen(_ouDatas->Path, "wb");
// 写入文件头
    if (isZIPs) {        
        fwrite(&_Heads->_Heads,   sizeof(char), sizeof(_Heads->_Heads),   hFiles);
        fwrite(&_Heads->_Version, sizeof(char), sizeof(_Heads->_Version), hFiles);
    }
// 写入数据
    fwrite(_ouDatas->Datas, sizeof(char), _ouDatas->Length, hFiles);
// 释放资源
    free(_ouDatas->Datas);
// 关闭文件
    fclose(hFiles);
}
/* ==================================
     创建新对象(输入文件路径，输出文件路径)
   ================================== */
void CHuffman::Init(char *inPath, char *outPath)
{
    int i;
    _inDatas = new Hm_DataBlocks();
    _ouDatas = new Hm_DataBlocks();

    for (i=0; i<strlen(inPath); i++)  _inDatas->Path[i]=inPath[i];
    _inDatas->Path[i] = '\0';

    for (i=0; i<strlen(outPath); i++) _ouDatas->Path[i]=outPath[i];
    _ouDatas->Path[i] = '\0';

    _Heads->_Heads[0] = 'z';
    _Heads->_Heads[1] = 'H';
    _Heads->_Heads[2] = 'A';
    _Heads->_Heads[3] = 'i';
    _Heads->_Heads[4] = '.';
    _Heads->_Heads[5] = 0;
    _Heads->_Version  = 1;

    _inDatas->Offset = 0;
    _inDatas->Length = 0;
    if (sizeof(_inDatas->Path)){  // strlen
        FILE *hFiles= fopen(_inDatas->Path, "rb");
        if ( hFiles ) {
            fseek(hFiles, 0, SEEK_END);       //rewind函数作用等同于 (void)fseek(stream, 0L, SEEK_SET);
            _inDatas->Length = ftell(hFiles); //文件大小
            rewind(hFiles);                  //将文件内部的位置指针重新指向一个流（数据流/文件）的开头

            _inDatas->Datas = (char*)malloc(sizeof(char)*_inDatas->Length);
            size_t result = fread(_inDatas->Datas, 1, _inDatas->Length, hFiles);
            if ( result != _inDatas->Length ) //文件读取失败！
            {
                free(_inDatas->Datas);
                fclose(hFiles);
                return;
            }
            fclose(hFiles);
        }
    }
    size_t nId;
    for( nId=0; nId<HM_TbCount; nId++ )
    {
        _HmTbs[nId] = new Hm_Tables();
        _HmTbs[nId]->InitDef();
    }
}

BOOL CHuffman::Encrypt(char *inPath, char *outPath)
{
    Init(inPath, outPath);
    do_ZIP();
    isZIPs = true;
    do_toFile();
    return _ouDatas->Length;
}

BOOL CHuffman::Decrypt(char *inPath, char *outPath)
{
    Init(inPath, outPath);
    do_UnZIP();
    isZIPs = false;
    do_toFile();
    return _ouDatas->Length;
}
