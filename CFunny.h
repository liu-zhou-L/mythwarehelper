#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<ctime>
#include<windows.h>
#include "assert.h"
#include <wincrypt.h>
namespace FunnyB{

class WinRandom {
	HCRYPTPROV handle;
public:
	WinRandom() {
		handle = NULL;
        CryptAcquireContext(
        	(HCRYPTPROV*)&handle,NULL,NULL,
            PROV_RSA_FULL,0
		);
	}
	~WinRandom() {
    	if (handle != NULL) CryptReleaseContext(handle, 0);
	}
    bool randBuf(void *dest, int len) {
    	if (handle == NULL) return false;
        return CryptGenRandom(handle, len, (BYTE*)dest);
	}
#   define _(func, typ) \
	typ func() { \
    	typ ret = 0; \
        assert(randBuf((void *)&ret, sizeof(ret))); \
        return ret; \
    }
    _(randInt, int)
    _(randLong, long long)
    _(randUnsigned, unsigned)
    _(randUnsignedLong, unsigned long long)
    _(randShort, short)
    _(randUnsignedShort, unsigned short)
	_(randChar, char)
    _(randUnsignedChar, unsigned char)
    _(randSignedChar, signed char)
};
	class Funny{
		public:
			long long random_F(int start,int end)
			{
				WinRandom T;
				int t=T.randLong()%(end+1);
				while(!(start<=t&&t<=end))
				{
					t=T.randLong()%(end+1);
				}
				return t;
			}
			char random_Char()
			{
				WinRandom T;
				char t=T.randChar()%('z'+1);
				while(!((t>='a'&&t<='z')||(t>='A'&&t<='Z')||((t>='1'&&t<='9'))))
				{
					t=T.randChar()%('z'+1);
				}
				return t;
			}
		
	};
int say(char *saying,int rate)
{
	FILE *open=fopen("1.vbs","w+");
	fprintf(open,"dim objSV\n");
	fprintf(open,"Set objSV = CreateObject(\"SAPI.SpVoice\")\n",saying);
	if(rate>=0)
	{
		fprintf(open,"objSV.Rate = %d\n",rate);
	}
	fprintf(open,"objSV.Speak(\"%s\")\n",saying);
	fclose(open);
	SHELLEXECUTEINFO info;
	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_FLAG_NO_UI;
	info.hwnd = NULL;
	info.lpVerb = ("Open");
	info.lpFile = ("1.vbs");//对应的文件路径
	info.lpParameters = ("");
	info.lpDirectory = ("");
	info.nShow = SW_SHOW;
	info.hInstApp = NULL;
	ShellExecuteEx(&info);
	//Sleep(strlen(saying)*100);
	Sleep(200);
	system("del 1.vbs y");
	return 0;
}
int say(int saying,int rate)
{
	FILE *open=fopen("1.vbs","w+");
	fprintf(open,"dim objSV\n");
	fprintf(open,"Set objSV = CreateObject(\"SAPI.SpVoice\")\n",saying);
	if(rate>=0)
	{
		fprintf(open,"objSV.Rate = %d\n",rate);
	}
	fprintf(open,"objSV.Speak(\"%d\")\n",saying);
	fclose(open);
	SHELLEXECUTEINFO info;
	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_FLAG_NO_UI;
	info.hwnd = NULL;
	info.lpVerb = ("Open");
	info.lpFile = ("1.vbs");//对应的文件路径
	info.lpParameters = ("");
	info.lpDirectory = ("");
	info.nShow = SW_SHOW;
	info.hInstApp = NULL;
	ShellExecuteEx(&info);
	//Sleep(strlen(saying)*100);
	Sleep(200);
	system("del 1.vbs y");
	return 0;
}
}

