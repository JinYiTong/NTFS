#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <fstream>
#include <string>
#include <Windows.h>
#include <atlstr.h>
//#define TEST
#define MAXVOL 3
using namespace std;

typedef struct _name_cur {
    CString filename;
    DWORDLONG pfrn;
} Name_Cur;

typedef struct _pfrn_name {
    DWORDLONG pfrn;
    CString filename;
} Pfrn_Name;
typedef map<DWORDLONG, Pfrn_Name> Frn_Pfrn_Name_Map;

class cmpStrStr {
public:
    cmpStrStr(bool uplow, bool inorder) {
        this->uplow = uplow;
        this->isOrder = inorder;
    }

    ~cmpStrStr() {};

    bool cmpStrFilename(CString str, CString filename);

    bool infilename(CString &strtmp, CString &filenametmp);

private:
    bool uplow;
    bool isOrder;
};


class Volume {
public:
    Volume(char vol) {
        this->vol = vol;
        hVol = NULL;
        path = "";
    }

    ~Volume() {
//		CloseHandle(hVol);
    }

    bool initVolume() {
        if (
            // 2.获取驱动盘句柄
                getHandle() &&
                createUSN() &&
                // 4.获取USN日志信息
                getUSNInfo() &&
                getUSNJournal() &&
                deleteUSN()
                ) {
            return true;
        } else {
            return false;
        }
    }

    bool isIgnore(vector<wstring> *pignorelist) {
        wstring tmp = path.GetBuffer();
        for (vector<wstring>::iterator it = pignorelist->begin();
             it != pignorelist->end(); ++it) {
            size_t i = it->length();
            if (!tmp.compare(0, i, *it, 0, i)) {
                return true;
            }
        }

        return false;
    }

    bool getHandle();

    bool createUSN();

    bool getUSNInfo();

    bool getUSNJournal();

    bool deleteUSN();

    vector<CString> findFile(CString str, cmpStrStr &cmpstrstr, vector<wstring> *pignorelist);

    CString getPath(DWORDLONG frn, CString &path);

    vector<CString> rightFile;     // 结果

private:
    char vol;
    HANDLE hVol;
    vector<Name_Cur> VecNameCur;        // 查找1
    Name_Cur nameCur;
    Pfrn_Name pfrnName;
    Frn_Pfrn_Name_Map frnPfrnNameMap;    // 查找2


    CString path;

    USN_JOURNAL_DATA ujd;
    CREATE_USN_JOURNAL_DATA cujd;
};

CString Volume::getPath(DWORDLONG frn, CString &path) {
    // 查找2
    Frn_Pfrn_Name_Map::iterator it = frnPfrnNameMap.find(frn);
    if (it != frnPfrnNameMap.end()) {
        if (0 != it->second.pfrn) {
            getPath(it->second.pfrn, path);
        }
        path += it->second.filename;
        path += (_T("\\"));
    }

    return path;
}

vector<CString> Volume::findFile(CString str, cmpStrStr &cmpstrstr, vector<wstring> *pignorelist) {

    // 遍历 VecNameCur
    // 通过一个匹配函数， 得到符合的 filename
    // 传入 frnPfrnNameMap，递归得到路径

    for (vector<Name_Cur>::const_iterator cit = VecNameCur.begin();
         cit != VecNameCur.end(); ++cit) {
        if (cmpstrstr.cmpStrFilename(str, cit->filename)) {
            path.Empty();
            // 还原 路径
            // vol:\  path \ cit->filename
            getPath(cit->pfrn, path);
            path += cit->filename;


            // path已是全路径

            if (isIgnore(pignorelist)) {
                continue;
            }
            rightFile.push_back(path);
            //path.Empty();
        }
    }

    return rightFile;
}

bool cmpStrStr::cmpStrFilename(CString str, CString filename) {
    // TODO 加入别的匹配函数

    int pos = 0;
    int end = str.GetLength();
    while (pos < end) {
        // 对于str，取得 每个空格分开为的关键词
        pos = str.Find(_T(' '));

        CString strtmp;
        if (pos == -1) {
            // 无空格
            strtmp = str;
            pos = end;
        } else {
            strtmp = str.Mid(0, pos - 0);
        }

        if (!infilename(strtmp, filename)) {
            return false;
        }
        str.Delete(0, pos);
        str.TrimLeft(' ');
    }

    return true;
}

bool cmpStrStr::infilename(CString &strtmp, CString &filename) {
    CString filenametmp(filename);
    int pos;
    if (!uplow) {
        // 大小写敏感
        filenametmp.MakeLower();
        pos = filenametmp.Find(strtmp.MakeLower());
    } else {
        pos = filenametmp.Find(strtmp);
    }

    if (-1 == pos) {
        return false;
    }
    if (!isOrder) {
        // 无顺序
        filename.Delete(0, pos + 1);
    }

    return true;
}

bool Volume::getHandle() {
    // 为\\.\C:的形式
    CString lpFileName(L"\\\\.\\c:");
    lpFileName.SetAt(4, vol);


    hVol = CreateFile(lpFileName,
                      GENERIC_READ | GENERIC_WRITE, // 可以为0
                      FILE_SHARE_READ | FILE_SHARE_WRITE, // 必须包含有FILE_SHARE_WRITE
                      NULL,
                      OPEN_EXISTING, // 必须包含OPEN_EXISTING, CREATE_ALWAYS可能会导致错误
                      FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL可能会导致错误
                      NULL);


    if (INVALID_HANDLE_VALUE != hVol) {
        return true;
    } else {
        printf("handler error %d\n", GetLastError());
        return false;
//		exit(1);

    }
}

bool Volume::createUSN() {
    cujd.MaximumSize = 0;
    cujd.AllocationDelta = 0;

    DWORD br;
    if (
            DeviceIoControl(hVol,// handle to volume
                            FSCTL_CREATE_USN_JOURNAL,      // dwIoControlCode
                            &cujd,           // input buffer
                            sizeof(cujd),         // size of input buffer
                            NULL,                          // lpOutBuffer
                            0,                             // nOutBufferSize
                            &br,     // number of bytes returned
                            NULL) // OVERLAPPED structure
            ) {
        return true;
    } else {
        printf("handler error %d\n", GetLastError());
        return false;
    }
}

bool Volume::getUSNInfo() {
    DWORD br;
    if (
            !DeviceIoControl(hVol, // handle to volume
                             FSCTL_QUERY_USN_JOURNAL,// dwIoControlCode
                             NULL,            // lpInBuffer
                             0,               // nInBufferSize
                             &ujd,     // output buffer
                             sizeof(ujd),  // size of output buffer
                             &br, // number of bytes returned
                             NULL) // OVERLAPPED structure
            ) {
        printf("getusninfo error %d", GetLastError());
        return false;
    }
}

bool Volume::getUSNJournal() {
    MFT_ENUM_DATA_V0 med;
    med.StartFileReferenceNumber = 0;
    med.LowUsn = ujd.FirstUsn;
    med.HighUsn = ujd.NextUsn;

    // 根目录
    CString tmp(_T("C:"));
    tmp.SetAt(0, vol);
    frnPfrnNameMap[0x5000000000005].filename = tmp;
    frnPfrnNameMap[0x5000000000005].pfrn = 0;

#define BUF_LEN 0x10000    // 尽可能地大，提高效率

    CHAR Buffer[BUF_LEN];
    DWORD usnDataSize;
    PUSN_RECORD UsnRecord;
    int USN_counter = 0;

    // 统计文件中...

    while (0 != DeviceIoControl(hVol,
                                FSCTL_ENUM_USN_DATA,
                                &med,
                                sizeof(med),
                                Buffer,
                                BUF_LEN,
                                &usnDataSize,
                                NULL)) {

        DWORD dwRetBytes = usnDataSize - sizeof(USN);
        // 找到第一个 USN 记录
        UsnRecord = (PUSN_RECORD) (((PCHAR) Buffer) + sizeof(USN));

        while (dwRetBytes > 0) {
            // 获取到的信息
            CString CfileName(UsnRecord->FileName, UsnRecord->FileNameLength / 2);

            pfrnName.filename = nameCur.filename = CfileName;
            pfrnName.pfrn = nameCur.pfrn = UsnRecord->ParentFileReferenceNumber;

            // Vector
            VecNameCur.push_back(nameCur);

            // 构建hash...
            frnPfrnNameMap[UsnRecord->FileReferenceNumber] = pfrnName;
            // 获取下一个记录
            DWORD recordLen = UsnRecord->RecordLength;
            dwRetBytes -= recordLen;
            UsnRecord = (PUSN_RECORD) (((PCHAR) UsnRecord) + recordLen);

        }
        // 获取下一页数据
        med.StartFileReferenceNumber = *(USN *) &Buffer;
    }

    return true;
}

bool Volume::deleteUSN() {
    DELETE_USN_JOURNAL_DATA dujd;
    dujd.UsnJournalID = ujd.UsnJournalID;
    dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;
    DWORD br;

    if (DeviceIoControl(hVol,
                        FSCTL_DELETE_USN_JOURNAL,
                        &dujd,
                        sizeof(dujd),
                        NULL,
                        0,
                        &br,
                        NULL)
            ) {
        CloseHandle(hVol);
        return true;
    } else {
        CloseHandle(hVol);
        return false;
    }
}