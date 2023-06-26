(Português)
/////////////////////////////////////////////////////
RisohEditor por katahiromz
/////////////////////////////////////////////////////

RisohEditor é um editor de recursos gratuito para desenvolvimento Win32.
Ele pode editar/extrair/clonar/excluir os dados do recurso em
Arquivos RC/RES/EXE/DLL.

Baixar binário: https://katahiromz.web.fc2.com/re/en

Funciona no Windows XP/2003/Vista/7/8.1/10 e ReactOS.

Consulte "LICENSE.txt" para obter detalhes sobre direitos autorais e
contrato de licença.

/////////////////////////////////////////////////////

Pergunta 1. O que é "Risoh"?

  Resposta. A palavra "Risoh" significa "ideal" em Japonês.

Pergunta 2. O que são edt1, edt2, cmb1?

  Resposta. Essas são macros de ID de controle padrão definidas em <dlgs.h>.

Pergunta 3. O que é mcdx?

  Resposta. É um compilador de mensagens especial que fiz.
            Consulte mcdx/MESSAGETABLEDX.md para obter detalhes.

Pergunta 4. Por que recebi caracteres distorcidos ao compilar com o Visual Studio?

  Resposta. O compilador de recursos do MSVC tem um bug no tratamento de
            Arquivos de recursos UTF-8.

            Use UTF-16 (mas UTF-16 não é suportado em GNU windres).

Pergunta 5. Qual é a diferença entre instalador e versão portátil?

  Resposta. A versão portátil não usa registro, mas um arquivo ini.

Pergunta 6. Os arquivos de 64 bits são suportados?

  Resposta. Sim no Windows 64 bits. No entanto, a camada de emulação WoW64 impede o 
            carregamento disso "C:\Program Files" ou "C:\Windows\system32".
            Você precisa copiar o arquivo de 64 bits para outro local antes de carregar.

/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Página Web (Inglês):   https://katahiromz.web.fc2.com/re/en
Página Web (Chinês):   https://katahiromz.web.fc2.com/re/ch
Página Web (Japonês):  https://katahiromz.web.fc2.com/re/ja
Página Web (Italiano): https://katahiromz.web.fc2.com/re/it
Página Web (Russo):    https://katahiromz.web.fc2.com/re/ru
Email                  katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
