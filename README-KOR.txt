(한국어)
/////////////////////////////////////////////////////
katahiromz가 개발한 RisohEditor
/////////////////////////////////////////////////////

RisohEditor는 Win32 개발을 위한 무료 리소스 편집기입니다.
RC/RES/EXE/DLL 파일의 리소스 데이터를 편집/추출/복제/삭제
할 수 있습니다.

바이너리 다운로드: https://katahiromz.web.fc2.com/re/ko/

Windows XP/2003/Vista/7/8.1/10 및 ReactOS에서 작동합니다.

저작권 및 라이센스 계약에 대한 자세한 내용은 
"LICENSE.txt"를 참조하십시오.

/////////////////////////////////////////////////////

질문 1. "Risoh"가 무엇입니까?

    대답. "Risoh"라는 단어는 일본어로 "이상적인"을 의미합니다.

질문 2. edt1, edt2, cmb1은 무엇입니까?

    대답. 이들은 <dlgs.h>에 정의된 표준 제어 ID 매크로입니다.

질문 3. mcdx란 무엇입니까?

    대답. 제가 만든 특별한 메시지 컴파일러입니다.
            자세한 내용은 mcdx/MESSAGETABLEDX.md를 참조하십시오.

질문 4. Visual Studio로 컴파일할 때 잘못된 문자가 표시되는 이유는 무엇입니까?

    대답. MSVC의 리소스 컴파일러는 UTF-8 리소스 파일을 처리하는데 
            버그가 있습니다.

            UTF-16을 사용합니다 (그러나 UTF-16은 GNU windres에서 지원되지 않습니다).

질문 5. 설치 프로그램 없음과 휴대용 버전의 차이점은 무엇입니까?

    대답. 휴대용 버전은 레지스트리를 사용하지 않고 ini 파일을 사용합니다.

질문 6. 64비트 파일이 지원됩니까?

    대답. 예. 64비트 Windows에서는 가능합니다. 그러나 WoW64 에뮬레이션 레이어는 
           "C:\Program Files" 또는 C:\Windows\system32"에서 로드되는 것을 방지합니다.
            로드하기 전에 64비트 파일을 다른 위치에 복사해야 합니다.

/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Webpage (Korean):   https://katahiromz.web.fc2.com/re/ko
Webpage (English):  https://katahiromz.web.fc2.com/re/en
Webpage (Chinese):  https://katahiromz.web.fc2.com/re/ch
Webpage (Japanese): https://katahiromz.web.fc2.com/re/ja
Webpage (Italian):  https://katahiromz.web.fc2.com/re/it
Webpage (Russian):  https://katahiromz.web.fc2.com/re/ru
Email               katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
