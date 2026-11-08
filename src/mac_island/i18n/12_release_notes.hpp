// What each published version brought (the settings' "What's new", newest
// first): a short summary for users, in each of the island's languages, in
// TextLanguage's order. A line starting with "#" is a heading, the others
// are items; an item can start with the option it tells about in braces
// (see ControlPanel::NoteTarget).
struct ReleaseNotes {
    PCWSTR version;
    PCWSTR text[kTextLanguages];
};
const ReleaseNotes kReleaseNotes[] = {
    {L"3.0.0",
     {
         L"#New\n"
         L"{Setup}A setup assistant (Setup assistant tab, under About) walks "
         L"through the main choices: the style, the pill's items, hiding, "
         L"notifications and the look, each shown live and undone by Cancel; "
         L"it opens by itself on a new install.\n"
         L"{IconPack}Icon packs (Icons tab): one of 12 sets (Fluent, Lucide, "
         L"Material, Phosphor, Emoji and more, downloaded once) or a folder "
         L"of your own pictures; any single icon can also get its own "
         L"picture or color.\n"
         L"{HideWhen}Choose when the island hides as its line (Hiding tab): "
         L"with the mouse away, only while a window is maximized, or only "
         L"while a window is over it; on the desktop it stays.\n"
         L"{HideClock}Only the clock when hidden (Hiding tab): instead of "
         L"the line, a small pill with the time stays; it becomes the line "
         L"while a window is under it (Island and Fixed styles). The Bar "
         L"hidden with Win+J closes into the same pill; the mouse on it "
         L"brings the bar back.\n"
         L"{LineFade}The line can fade away after 30 s, 2 min or 10 min "
         L"alone (Hiding tab), so it doesn't stay on OLED screens; the mouse "
         L"or a notification brings it back.\n"
         L"{NotchOpen}With notches, choose which notch's line opens the "
         L"hidden island (Style tab); the other lines are hidden and clicks "
         L"there go to the windows below.\n"
         L"{Notches}Notches in the Island style too (Style tab), like the "
         L"Fixed style's but without keeping a strip of the screen; each "
         L"style keeps its own choice.\n"
         L"{BarSide}The Bar style on the left or the right edge too (Look "
         L"tab), its items one under the other, with its inner corners "
         L"rounded where it meets the screen.\n"
         L"{BarClear}A see-through bar (Look tab): only its items over "
         L"what's behind it, or, with the frosted glass, only the blur.\n"
         L"{TextSize}The text size has its own slider (Look tab), apart from "
         L"the icons' size.\n"
         L"{AiUsage}AI usage on the pill (Apps and tools tab): how much of "
         L"Claude Code's (in a terminal) and Codex's limits is used and when "
         L"they start over, read on this PC with no login and nothing online.\n"
         L"{Commands}Your commands (Apps and tools tab): shortcuts to "
         L"programs, files, folders, links or command lines, with their own "
         L"icons, in a panel; any of them can also be a button on the pill.\n"
         L"{FocusedTitle}The app in focus can show its window's title beside "
         L"its name (Desktop tab).\n"
         L"{CalendarBigDay}A calendar card (Clock and calendar tab): the day "
         L"big beside the month, buttons for an alarm, a timer and your "
         L"agendas, the day's events and what's coming next.\n"
         L"{WeatherCards}The weather's details as cards, like a phone's "
         L"(Apps and tools tab, under Weather).\n"
         L"{WeatherUnit}Celsius or Fahrenheit for the weather (Apps and "
         L"tools tab, under Weather).\n"
         L"{CapsLock}A bubble saying when Caps Lock turns on or off (Banners "
         L"tab).\n"
         L"{Profiles}Profiles that turn on by themselves (General tab, a "
         L"profile's pencil): with some apps in front or with a style "
         L"chosen; after its app, it goes back to the profile before or stays.\n"
         L"The mobile hotspot's name and password can be changed in the "
         L"control center, on the hotspot's page.\n"
         L"{Dock}The dock can stay up on the desktop and hide only while a "
         L"window is maximized or over it, like the island (Windhawk's "
         L"settings).\n"
         L"{Dock}The dock can stop at a width you choose; the other apps "
         L"scroll with the mouse wheel, like a box (Windhawk's settings).\n"
         L"{Dock}The dock's magnification, bounces and \"When it hides\" "
         L"also on the Windows taskbar kept with only its apps (an option in "
         L"Windhawk's settings).\n"
         L"{BannerPlace}Banners at one of nine points of the screen, picked "
         L"on a little screen (Banners tab), sliding in from the nearest "
         L"edge; or out of the pill, as before.\n"
         L"{Shelf}A shelf for files (Desktop tab): drop files anywhere on "
         L"the pill to keep them there (only their places, not copies), then "
         L"open its icon and drag them into another app.\n"
         L"{FolderThumbnails}Thumbnails of pictures, videos and documents in "
         L"the folders panel and on the shelf (Desktop tab); rest the mouse "
         L"on one for half a second for a bigger preview.\n"
         L"{TimerLine}While a timer, Pomodoro or the stopwatch runs with the "
         L"island minimized, the line becomes a small pill with its ring and "
         L"time (Hiding tab); with Only the clock, the time goes beside the "
         L"clock.\n"
         L"{LineClick}The hidden line can open with a click instead of the "
         L"mouse (Hiding tab): it only grows under the mouse, so the island "
         L"doesn't open by accident; the hidden Bar's clock too.\n"
         L"The weather can use your location (\"Use my location\" in the "
         L"weather panel), with Windows' own permission; it goes to the "
         L"weather service rounded to about a kilometer, and typing a city "
         L"goes back to it.\n"
         L"Alarms can have a name and the days they ring: every day, "
         L"weekdays, weekends, any days, or once (it turns itself off after "
         L"ringing).\n"
         L"{GlassLight}Light and shadow like macOS 27 for the Liquid Glass "
         L"(Look tab): a sheen, a beveled edge, light along its bottom and a "
         L"soft shadow, and the panels' cards look like glass too. An idea "
         L"by Thiago Silva.\n"
         L"The island speaks Vietnamese (Tiếng Việt), translated by caotu2k5.\n"
         L"#Improvements\n"
         L"The settings window looks like Windows 11's: its own title bar, "
         L"bigger texts, the small options two on a line in a wide window, and "
         L"scroll bars always shown that can be dragged (the side bar's too).\n"
         L"The bubbles that ask or tell something (an option's needs, the "
         L"icons' tips, the settings bubble) are opaque whatever the island's "
         L"opacity.\n"
         L"The mod's site, island.caliberda.com.br (About tab): every option "
         L"explained, the roadmap and your votes.\n"
         L"{Style}Settings reorganized: new Style, Hiding, Control center "
         L"and Banners tabs, with the options that depend on each other "
         L"together.\n"
         L"A dimmed option says why; a click on that line shows everything "
         L"it needs, \"Yes, change\" turns it all on at once, and \"Restore "
         L"what changed\" puts it back.\n"
         L"Choices never cut their names (they go to more rows when needed), "
         L"and places and corners are picked on drawings.\n"
         L"In What's new, a feature with an arrow lights under the mouse, "
         L"and a click on it shows its option.\n"
         L"{FolderDrag}Files dragged out of the folders panel or the shelf: "
         L"asked the first time (then remembered), asked every time, copied "
         L"or moved (Desktop tab).\n"
         L"{MinimizeKey}Win+J (Hiding tab) hides the whole bar in the Bar "
         L"style too, and brings it back.\n"
         L"{ExternalBrightness}The brightness of external monitors over "
         L"their cable (DDC/CI) can be turned off (Control center tab), for "
         L"monitors that flicker or docks and MST hubs that reset when the "
         L"control center opens.\n"
         L"{BannerSize}The banner's size (50% to 200%), width and corners "
         L"can be chosen (Banners tab).\n"
         L"{DndSilent}Calls, alarms and reminders can stay silent in Do not "
         L"disturb too (Notifications tab).\n"
         L"{DownloadStalled}A browser's download standing still for 30 s "
         L"shows as paused, in yellow (System tab): browsers never say a "
         L"download is paused.\n"
         L"{Reset}A Reset button beside Export and Import (General tab, a "
         L"double click) puts the island's settings back as they came; "
         L"notes, tasks, events and profiles stay.\n"
         L"The control center's mixer shows each app's volume in percent.\n"
         L"Pages opened straight from the pill have a back arrow.\n"
         L"The event editor says what its reminder will do.\n"
         L"The calendar card names the month its days show, over them.\n"
         L"#Fixes\n"
         L"The dock keeps the icons set on the shortcuts pinned to the taskbar "
         L"instead of the app's own (#6891).\n"
         L"With a monitor of another scale below, the dock is no longer cut "
         L"off center after its taskbar mode is switched.\n"
         L"Alt or F10 in the settings window, or moving it, no longer stops "
         L"the islands' animations.\n"
         L"The tray icons of programs the island can't open (like NVIDIA's) "
         L"keep their picture after the mod is turned off and on.\n"
         L"The weather panel on another monitor's island no longer stays on "
         L"Searching.\n"
         L"\"Restore what changed\" stays after the islands start again, and "
         L"leaves an option changed again since.\n"
         L"The brightness, Caps Lock with three notches, the shelf's count and "
         L"\"Window over it\" on other monitors' docks work as they should.\n"
         L"With Windhawk run as administrator, the other apps' tray icons, "
         L"their badges and the download progress show on the island again.\n"
         L"With animations off, panels open without the pill flickering, and "
         L"the power button's confirmation shows.\n"
         L"Shortcuts and notes keep their line breaks in Windhawk portable.\n"
         L"Song lyrics for Apple Music (the app, Cider and its web page) and "
         L"for players that don't tell a song's length.\n"
         L"Apps with notifications seen on one island (a notch or another "
         L"monitor's) are seen on all of them.\n"
         L"No white band under the dock or the see-through taskbar after the "
         L"taskbar hides by itself.\n"
         L"The island no longer goes empty with graphics drivers that can't "
         L"make pictures smaller the smooth way.\n"
         L"In the Bar style with the clock in the middle, items no longer "
         L"flash over the clock.\n"
         L"The Recycle Bin's count keeps up when Explorer starts after the "
         L"island.\n"
         L"Changing the power mode in the control center no longer freezes "
         L"the island.\n"
         L"The bar moved to a side gives the screen's top back (and the "
         L"side, when it moves again).\n"
         L"On an HDR monitor, its brightness slider changes the desktop's "
         L"brightness (Windows' brightness of SDR content).\n"
         L"The places and icons per app go back to the general ones on the "
         L"desktop and the taskbar, instead of staying the last app's.\n"
         L"Over another monitor below, the hidden dock slides away past the "
         L"screen's edge, instead of shrinking into a line above it.\n"
         L"The 中/英 item follows Shift with input methods that tell "
         L"nothing else, like the newest Microsoft Pinyin.\n"
         L"An app's own icons, chosen before some items existed, no longer "
         L"show those items while they're off.\n"
         L"The control center's usage page shows CPU, RAM and VRAM "
         L"without their items on the pill, with a tab for each.\n"
         L"With two monitors, an island stays hidden while its own "
         L"monitor has a full-screen window, whatever window has the "
         L"focus.\n"
         L"A keyboard switched in the same app shows on the language "
         L"item.\n"
         L"The dock comes up when the mouse slides along the bottom edge "
         L"into it.\n"
         L"A click on a desktop app's notification goes where it points, "
         L"for the apps that register it.\n"
         L"The panels follow the pill's corners, and the pill no longer "
         L"changes after a screenshot.\n"
         L"An app clicked in the dock bounces from the click until its "
         L"window shows.",
         L"#Novidades\n"
         L"{Setup}Um assistente de configuração (aba Assistente de "
         L"configuração, abaixo de Sobre) percorre as escolhas principais: o "
         L"estilo, os itens da pílula, o esconder, as notificações e a "
         L"aparência, cada uma mostrada ao vivo e desfeita por Cancelar; ele "
         L"abre sozinho numa instalação nova.\n"
         L"{IconPack}Pacotes de ícones (aba Ícones): um de 12 conjuntos "
         L"(Fluent, Lucide, Material, Phosphor, Emoji e mais, baixados uma "
         L"vez) ou uma pasta com as suas imagens; qualquer ícone também pode "
         L"ter uma imagem ou uma cor só dele.\n"
         L"{HideWhen}Escolha quando a ilha vira a linha (aba Esconder): com "
         L"o mouse longe, só enquanto uma janela está maximizada, ou só "
         L"enquanto uma janela está por cima; na área de trabalho ela fica.\n"
         L"{HideClock}Só o relógio ao esconder (aba Esconder): no lugar da "
         L"linha fica uma pílula pequena com a hora; ela vira a linha "
         L"enquanto uma janela está embaixo (estilos Ilha e Fixa). A Barra "
         L"escondida com Win+J vira a mesma pílula; o mouse em cima traz a "
         L"barra de volta.\n"
         L"{LineFade}A linha pode sumir depois de 30 s, 2 min ou 10 min "
         L"parada (aba Esconder), para não ficar marcada em telas OLED; o "
         L"mouse ou uma notificação traz de volta.\n"
         L"{NotchOpen}Com entalhes, escolha a linha de qual entalhe abre a "
         L"ilha escondida (aba Estilo); as outras linhas somem e os cliques "
         L"ali vão para as janelas de baixo.\n"
         L"{Notches}Entalhes também no estilo Ilha (aba Estilo), como os da "
         L"Fixa, mas sem reservar uma faixa da tela; cada estilo guarda a "
         L"sua escolha.\n"
         L"{BarSide}O estilo Barra também na borda esquerda ou direita (aba "
         L"Aparência), com os itens um embaixo do outro e os cantos de "
         L"dentro arredondados onde ela encontra a tela.\n"
         L"{BarClear}Uma barra transparente (aba Aparência): só os itens por "
         L"cima do que está atrás, ou, com o vidro fosco, só o desfoque.\n"
         L"{TextSize}O tamanho dos textos tem um controle próprio (aba "
         L"Aparência), separado do tamanho dos ícones.\n"
         L"{AiUsage}Uso de IA na pílula (aba Apps e ferramentas): quanto dos "
         L"limites do Claude Code (num terminal) e do Codex já foi usado e "
         L"quando recomeçam, lido neste PC, sem login e sem nada online.\n"
         L"{Commands}Seus comandos (aba Apps e ferramentas): atalhos para "
         L"programas, arquivos, pastas, links ou linhas de comando, com os "
         L"próprios ícones, num painel; qualquer um também pode virar um "
         L"botão na pílula.\n"
         L"{FocusedTitle}O app em foco pode mostrar o título da janela ao "
         L"lado do nome (aba Área de trabalho).\n"
         L"{CalendarBigDay}Um cartão do calendário (aba Relógio e "
         L"calendário): o dia grande ao lado do mês, botões para alarme, "
         L"timer e suas agendas, os eventos do dia e o que vem por aí.\n"
         L"{WeatherCards}Os detalhes do clima em cartões, como no celular "
         L"(aba Apps e ferramentas, em Clima).\n"
         L"{WeatherUnit}Celsius ou Fahrenheit para o clima (aba Apps e "
         L"ferramentas, em Clima).\n"
         L"{CapsLock}Um balão avisando quando o Caps Lock liga ou desliga "
         L"(aba Avisos).\n"
         L"{Profiles}Perfis que ligam sozinhos (aba Geral, no lápis do "
         L"perfil): com alguns apps na frente ou com um estilo escolhido; "
         L"depois do app, volta para o perfil de antes ou fica.\n"
         L"O nome e a senha do hotspot móvel podem ser mudados na central de "
         L"controle, na página do hotspot.\n"
         L"{Dock}A dock pode ficar à mostra na área de trabalho e esconder "
         L"só enquanto uma janela está maximizada ou por cima, como a ilha "
         L"(configurações do Windhawk).\n"
         L"{Dock}A dock pode parar numa largura que você escolhe; os outros "
         L"apps rolam com a roda do mouse, como uma caixa (configurações do "
         L"Windhawk).\n"
         L"{Dock}A ampliação, os pulos e o \"Quando esconder\" da dock "
         L"também na barra do Windows mantida só com os apps (uma opção nas "
         L"configurações do Windhawk).\n"
         L"{BannerPlace}Avisos num de nove pontos da tela, escolhido numa "
         L"telinha (aba Avisos), entrando pela borda mais perto; ou saindo "
         L"da pílula, como antes.\n"
         L"{Shelf}Uma prateleira de arquivos (aba Área de trabalho): solte "
         L"arquivos em qualquer lugar da pílula para guardá-los ali (só o "
         L"lugar deles, não cópias), depois abra o ícone dela e arraste para "
         L"outro app.\n"
         L"{FolderThumbnails}Miniaturas de fotos, vídeos e documentos no "
         L"painel de pastas e na prateleira (aba Área de trabalho); pare o "
         L"mouse meio segundo em cima de uma para ver uma prévia maior.\n"
         L"{TimerLine}Enquanto um timer, o Pomodoro ou o cronômetro corre "
         L"com a ilha minimizada, a linha vira uma pílula pequena com o anel "
         L"e o tempo (aba Esconder); com Só o relógio, o tempo fica ao lado "
         L"da hora.\n"
         L"{LineClick}A linha escondida pode abrir com um clique em vez do "
         L"mouse (aba Esconder): ela só cresce sob o mouse, para a ilha não "
         L"abrir sem querer; o relógio da Barra escondida também.\n"
         L"O clima pode usar a sua localização (\"Usar minha localização\" "
         L"no painel do clima), com a permissão do próprio Windows; ela vai "
         L"ao serviço do clima arredondada para cerca de um quilômetro, e "
         L"digitar uma cidade volta para ela.\n"
         L"Alarmes podem ter nome e os dias em que tocam: todos os dias, "
         L"dias úteis, fins de semana, quaisquer dias, ou uma vez (desliga "
         L"sozinho depois de tocar).\n"
         L"{GlassLight}Luz e sombra como no macOS 27 para o Liquid Glass "
         L"(aba Aparência): brilho, borda chanfrada, luz embaixo e uma "
         L"sombra suave, e os cartões dos painéis também parecem vidro. "
         L"Ideia do Thiago Silva.\n"
         L"A ilha fala vietnamita (Tiếng Việt), traduzido por caotu2k5.\n"
         L"#Melhorias\n"
         L"A janela de configurações tem a cara do Windows 11: barra de título "
         L"própria, textos maiores, as opções pequenas duas por linha numa "
         L"janela larga e barras de rolagem sempre visíveis que dá para "
         L"arrastar (a da barra lateral também).\n"
         L"Os balões que perguntam ou avisam algo (o que uma opção precisa, as "
         L"dicas dos ícones, o balão das configurações) são opacos, seja qual "
         L"for a opacidade da ilha.\n"
         L"O site do mod, island.caliberda.com.br (aba Sobre): cada opção "
         L"explicada, o roadmap e os seus votos.\n"
         L"{Style}Configurações reorganizadas: abas novas Estilo, Esconder, "
         L"Central de controle e Avisos, com as opções que dependem uma da "
         L"outra juntas.\n"
         L"Uma opção apagada diz o porquê; um clique nessa linha mostra tudo "
         L"de que ela precisa, \"Sim, alterar\" liga tudo de uma vez e "
         L"\"Restaurar o que mudou\" desfaz.\n"
         L"As escolhas nunca cortam os nomes (vão para mais linhas quando "
         L"precisa), e lugares e cantos são escolhidos em desenhos.\n"
         L"Em Novidades, um recurso com seta acende com o mouse em cima, e "
         L"um clique nele mostra a opção dele.\n"
         L"{FolderDrag}Arquivos arrastados para fora do painel de pastas ou "
         L"da prateleira: perguntado na primeira vez (e lembrado), "
         L"perguntado sempre, copiados ou movidos (aba Área de trabalho).\n"
         L"{MinimizeKey}O Win+J (aba Esconder) esconde a barra inteira "
         L"também no estilo Barra, e a traz de volta.\n"
         L"{ExternalBrightness}O brilho de monitores externos pelo cabo "
         L"(DDC/CI) pode ser desligado (aba Central de controle), para "
         L"monitores que piscam ou docks e hubs MST que reiniciam quando a "
         L"central abre.\n"
         L"{BannerSize}Dá para escolher o tamanho (50% a 200%), a largura e "
         L"os cantos do aviso (aba Avisos).\n"
         L"{DndSilent}Chamadas, alarmes e lembretes também podem ficar em "
         L"silêncio no Não incomodar (aba Notificações).\n"
         L"{DownloadStalled}Um download do navegador parado por 30 s aparece "
         L"pausado, em amarelo (aba Sistema): os navegadores nunca avisam "
         L"que um download pausou.\n"
         L"{Reset}Um botão Redefinir ao lado de Exportar e Importar (aba "
         L"Geral, com clique duplo) volta as configurações da ilha como "
         L"vieram; anotações, tarefas, eventos e perfis ficam.\n"
         L"O mixer da central de controle mostra o volume de cada app em "
         L"porcentagem.\n"
         L"As páginas abertas direto da pílula têm uma seta para voltar.\n"
         L"O editor de evento diz o que o lembrete dele vai fazer.\n"
         L"O cartão do calendário mostra o nome do mês dos dias, acima "
         L"deles.\n"
         L"#Correções\n"
         L"A dock mantém os ícones definidos nos atalhos fixados na barra de "
         L"tarefas, em vez do ícone do próprio app (#6891).\n"
         L"Com um monitor de outra escala embaixo, a dock não fica mais "
         L"cortada fora do centro depois de trocar o modo da barra.\n"
         L"Alt ou F10 na janela de configurações, ou movê-la, não param mais "
         L"as animações das ilhas.\n"
         L"Os ícones da bandeja de programas que a ilha não consegue abrir "
         L"(como o da NVIDIA) mantêm a imagem depois de desligar e ligar o "
         L"mod.\n"
         L"O painel do clima na ilha de outro monitor não fica mais em "
         L"Procurando.\n"
         L"\"Restaurar o que mudou\" continua depois que as ilhas reiniciam, e "
         L"não mexe numa opção mudada de novo depois.\n"
         L"O brilho, o Caps Lock com três entalhes, a contagem da prateleira e "
         L"\"Janela por cima\" nas docks dos outros monitores funcionam como "
         L"deveriam.\n"
         L"Com o Windhawk como administrador, os ícones da bandeja dos "
         L"outros apps, os contadores deles e o progresso dos downloads "
         L"voltam a aparecer na ilha.\n"
         L"Com as animações desligadas, os painéis abrem sem a pílula "
         L"piscar, e a confirmação do botão de desligar aparece.\n"
         L"Atalhos e anotações mantêm as quebras de linha no Windhawk portátil.\n"
         L"Letras de música para o Apple Music (o app, o Cider e a página "
         L"web) e para players que não informam a duração da música.\n"
         L"Apps com notificações vistos numa ilha (um entalhe ou a de outro "
         L"monitor) ficam vistos em todas.\n"
         L"Sem a faixa branca embaixo da dock ou da barra transparente "
         L"depois que a barra se esconde sozinha.\n"
         L"A ilha não fica mais vazia com drivers de vídeo que não conseguem "
         L"diminuir imagens do jeito suave.\n"
         L"No estilo Barra com o relógio no meio, os itens não piscam mais "
         L"por cima do relógio.\n"
         L"A contagem da Lixeira acompanha quando o Explorador começa depois "
         L"da ilha.\n"
         L"Trocar o modo de energia na central de controle não trava mais a "
         L"ilha.\n"
         L"A barra levada para uma lateral devolve o topo da tela (e a "
         L"lateral, quando muda de novo).\n"
         L"Num monitor com HDR, o controle de brilho muda o brilho da área "
         L"de trabalho (o brilho de conteúdo SDR do Windows).\n"
         L"Os lugares e ícones por app voltam aos gerais na área de trabalho "
         L"e na barra de tarefas, em vez de ficarem os do último app.\n"
         L"Com outro monitor embaixo, a dock escondida desliza para fora "
         L"pela borda da tela, em vez de encolher numa linha acima dela.\n"
         L"O item 中/英 acompanha o Shift com métodos de entrada que não "
         L"avisam mais nada, como o Microsoft Pinyin mais novo.\n"
         L"Os ícones próprios de um app, escolhidos antes de alguns itens "
         L"existirem, não mostram mais esses itens enquanto estão desligados.\n"
         L"A página de uso da central de controle mostra CPU, RAM e VRAM "
         L"sem esses itens na pílula, com uma aba para cada.\n"
         L"Com dois monitores, a ilha fica escondida enquanto o monitor "
         L"dela tem uma janela em tela cheia, seja qual for a janela com "
         L"o foco.\n"
         L"Um teclado trocado no mesmo app aparece no item do idioma.\n"
         L"A dock sobe quando o mouse desliza pela borda de baixo até "
         L"ela.\n"
         L"Um clique na notificação de um app de desktop vai para onde "
         L"ela aponta, nos apps que registram isso.\n"
         L"Os painéis seguem os cantos da pílula, e a pílula não muda "
         L"mais depois de um print.\n"
         L"Um app clicado na dock pula desde o clique até a janela "
         L"aparecer.",
         L"#新功能\n"
         L"{Setup}设置向导（“设置向导”选项卡，位于“关于”之下）带你走完主要选项：样式、胶囊上的项目、隐藏、通知和外观，"
         L"每项都会实时显示，点“取消”即可撤销；全新安装时它会自动打开。\n"
         L"{IconPack}图标包（“图标”选项卡）：12 "
         L"套图标之一（Fluent、Lucide、Material、Phosphor、Emoji "
         L"等，只需下载一次）或你自己的图片文件夹；任何单个图标也可以有自己的图片或颜色。\n"
         L"{HideWhen}选择灵动岛何时变成细线（“隐藏”选项卡）：鼠标离开时、仅在有窗口最大化时，或仅在有窗口覆盖它时；在桌面上它会保持"
         L"显示。\n"
         L"{HideClock}隐藏时只显示时钟（“隐藏”选项卡）：用带时间的小胶囊代替细线；有窗口在其下方时它会变回细线（灵动岛和固定样式）"
         L"。用 Win+J 隐藏的条形会收成同样的小胶囊；鼠标移到上面即可让条形回来。\n"
         L"{LineFade}细线闲置 30 秒、2 分钟或 10 分钟后可淡出（“隐藏”选项卡），避免在 OLED "
         L"屏幕上留下残影；鼠标或通知会让它回来。\n"
         L"{NotchOpen}使用刘海时，选择哪个刘海的细线打开隐藏的灵动岛（“样式”选项卡）；其他细线会隐藏，点击那里会传给下方的窗口。\n"
         L"{Notches}灵动岛样式也可以使用刘海（“样式”选项卡），与固定样式相同，但不保留屏幕条带；每种样式保留自己的选择。\n"
         L"{BarSide}条形样式也可放在左侧或右侧边缘（“外观”选项卡），项目上下排列，与屏幕相接的内侧角可变圆。\n"
         L"{BarClear}透明栏（“外观”选项卡）：只显示其项目，或在磨砂玻璃下只显示模糊效果。\n"
         L"{TextSize}文字大小有自己的滑块（“外观”选项卡），与图标大小分开。\n"
         L"{AiUsage}胶囊上的 AI 用量（“应用和工具”选项卡）：Claude Code（终端中）和 Codex "
         L"的额度已用多少、何时重置，在本机读取，无需登录，不联网。\n"
         L"{Commands}你的命令（“应用和工具”选项卡）：程序、文件、文件夹、链接或命令行的快捷方式，带有自己的图标，放在一个面板中；任"
         L"何一个也可以成为胶囊上的按钮。\n"
         L"{FocusedTitle}焦点应用可以在名称旁显示窗口标题（“桌面”选项卡）。\n"
         L"{CalendarBigDay}日历卡片（“时钟和日历”选项卡）：大号日期显示在月份旁，带有闹钟、计时器和日程按钮、当天的事件以及接"
         L"下来的安排。\n"
         L"{WeatherCards}天气详情以卡片显示，就像手机上那样（“应用和工具”选项卡，“天气”下）。\n"
         L"{WeatherUnit}天气可使用摄氏度或华氏度（“应用和工具”选项卡，“天气”下）。\n"
         L"{CapsLock}大写锁定打开或关闭时显示一个气泡（“横幅”选项卡）。\n"
         L"{Profiles}自动启用的配置文件（“常规”选项卡，配置文件的铅笔）：某些应用位于前台或选择某种样式时启用；该应用之后，回到之前"
         L"的配置文件或保持不变。\n"
         L"可以在控制中心的热点页面更改移动热点的名称和密码。\n"
         L"{Dock}程序坞可以在桌面上保持显示，仅在有窗口最大化或覆盖它时隐藏，与灵动岛一样（Windhawk 设置）。\n"
         L"{Dock}程序坞可以停在你选择的宽度；其余应用用鼠标滚轮滚动，像一个收纳盒（Windhawk 设置）。\n"
         L"{Dock}程序坞的放大、跳动和“何时隐藏”也可用于只保留应用的 Windows 任务栏（Windhawk 设置中的一个选项）。\n"
         L"{BannerPlace}横幅可显示在屏幕的九个位置之一，在一个小屏幕上选择（“横幅”选项卡），从最近的边缘滑入；或像以前一样从胶囊"
         L"中弹出。\n"
         L"{Shelf}文件暂存架（“桌面”选项卡）：把文件拖放到胶囊的任何位置保存在那里（只保存位置，不复制），然后打开它的图标，把文件拖入"
         L"其他应用。\n"
         L"{FolderThumbnails}文件夹面板和暂存架中显示图片、视频和文档的缩略图（“桌面”选项卡）；鼠标停留半秒可查看更大的预览。\n"
         L"{TimerLine}灵动岛最小化时，如果计时器、番茄钟或秒表正在运行，细线会变成一个带圆环和时间的小胶囊（“隐藏”选项卡）；开启“"
         L"隐藏时只显示时钟”时，时间显示在时钟旁边。\n"
         L"{LineClick}隐藏后的细线可以改为点击打开，而不是鼠标移上去就打开（“隐藏”选项卡）：鼠标移上去只会变粗，避免误开；隐藏的栏"
         L"的时钟也一样。\n"
         L"天气可以使用你的位置（天气面板中的“使用我的位置”），需要 Windows "
         L"自己的权限；位置会四舍五入到约一公里后才发送给天气服务，输入城市即可改回城市。\n"
         L"闹钟可以设置名称和响铃日期：每天、工作日、周末、任意几天，或仅一次（响铃后自动关闭）。\n"
         L"{GlassLight}为 Liquid Glass 提供类似 macOS 27 "
         L"的光影（“外观”选项卡）：高光、斜面边缘、底部的反光和柔和的阴影，面板中的卡片也呈现玻璃效果。创意来自 Thiago Silva。\n"
         L"灵动岛支持越南语（Tiếng Việt），由 caotu2k5 翻译。\n"
         L"#改进\n"
         L"设置窗口改为 Windows 11 的样子：自己的标题栏、更大的文字，窗口较宽时小选项每行两个，滚动条始终显示并可拖动（侧栏的也一样）。\n"
         L"询问或提示内容的气泡（选项需要的更改、图标的提示、设置气泡）不再随灵动岛的不透明度变透明。\n"
         L"模组网站 island.caliberda.com.br（“关于”选项卡）：每个选项的说明、路线图和你的投票。\n"
         L"{Style}设置已重新整理：新增“样式”“隐藏”“控制中心”和“横幅”选项卡，相互依赖的选项放在一起。\n"
         L"变暗的选项会说明原因；单击这行会显示它需要的一切，“是，更改”一次全部开启，“恢复所做的更改”可以撤销。\n"
         L"选项名称不再被截断（需要时换到更多行），位置和圆角通过图示选择。\n"
         L"在“新功能”中，带箭头的功能在鼠标移上去时会高亮，点击即可显示它的选项。\n"
         L"{FolderDrag}从文件夹面板或暂存架拖出的文件：第一次时询问（之后记住）、每次都询问、复制或移动（“桌面”选项卡）。\n"
         L"{MinimizeKey}Win+J（“隐藏”选项卡）在条形样式中也会隐藏整个栏，并让它回来。\n"
         L"{ExternalBrightness}可以关闭通过线缆（DDC/CI）调节外接显示器亮度（“控制中心”选项卡），适用于打开控制中心"
         L"时显示器闪烁或扩展坞、MST 集线器重置的情况。\n"
         L"{BannerSize}可以选择横幅的大小（50% 到 200%）、宽度和圆角（“横幅”选项卡）。\n"
         L"{DndSilent}来电、闹钟和提醒也可以在勿扰模式下保持静音（“通知”选项卡）。\n"
         L"{DownloadStalled}浏览器下载停滞 30 秒后显示为已暂停（黄色）（“系统”选项卡）：浏览器从不告知下载已暂停。\n"
         L"{Reset}导出和导入旁的“重置”按钮（“常规”选项卡，双击）将灵动岛设置恢复为默认；笔记、任务、事件和配置文件保留。\n"
         L"控制中心的混音器以百分比显示每个应用的音量。\n"
         L"从胶囊直接打开的页面带有返回箭头。\n"
         L"事件编辑器会说明其提醒将做什么。\n"
         L"日历卡片会在日期上方显示所显示的月份。\n"
         L"#修复\n"
         L"程序坞保留任务栏固定快捷方式上设置的图标，而不是换回应用自己的图标 (#6891)。\n"
         L"下方有不同缩放比例的显示器时，切换任务栏模式后程序坞不再偏离中心被裁切。\n"
         L"在设置窗口中按 Alt 或 F10，或移动窗口，不再让灵动岛的动画停住。\n"
         L"灵动岛无法打开的程序（如 NVIDIA）的托盘图标，在关闭再打开模组后仍保留图片。\n"
         L"其他显示器上灵动岛的天气面板不再一直停在“正在搜索”。\n"
         L"“恢复所做的更改”在灵动岛重新启动后仍在，并且不会改动之后又被更改过的选项。\n"
         L"亮度、三个刘海时的大写锁定、暂存架的数量，以及其他显示器程序坞上的“窗口覆盖”都已正常工作。\n"
         L"以管理员身份运行 Windhawk 时，其他应用的托盘图标、徽章和下载进度会再次显示在灵动岛上。\n"
         L"关闭动画时，面板打开时胶囊不再闪烁，关机按钮的确认会显示。\n"
         L"在便携版 Windhawk 中，快捷方式和笔记会保留换行。\n"
         L"支持 Apple Music（应用、Cider 及其网页）和不提供歌曲时长的播放器的歌词。\n"
         L"在一个灵动岛（刘海或其他显示器上的）上看过通知的应用，在所有灵动岛上都会标为已看。\n"
         L"任务栏自动隐藏后，程序坞或透明任务栏下方不再出现白色条带。\n"
         L"在无法平滑缩小图片的显卡驱动上，灵动岛不再变成空白。\n"
         L"在时钟居中的条形样式中，项目不再在时钟上方闪烁。\n"
         L"资源管理器晚于灵动岛启动时，回收站计数也会更新。\n"
         L"在控制中心更改电源模式不再导致灵动岛卡住。\n"
         L"移到侧边的栏会归还屏幕顶部（再次移动时也会归还侧边）。\n"
         L"在 HDR 显示器上，亮度滑块会改变桌面的亮度（Windows 的 SDR 内容亮度）。\n"
         L"每个应用的位置和图标在桌面和任务栏上会恢复为通用设置，而不是保持上一个应用的设置。\n"
         L"下方还有显示器时，隐藏的程序坞会滑出屏幕边缘，而不是缩成边缘上方的一条线。\n"
         L"对于不发出其他通知的输入法（如最新的微软拼音），「中/英」项目会跟随 Shift 变化。\n"
         L"应用自己的图标选择若早于某些项目，这些项目关闭时不再显示。\n"
         L"控制中心的使用情况页面在胶囊上没有这些项目时也会显示 CPU、内存和显存，每项一个标签页。\n"
         L"双显示器时，只要灵动岛所在的显示器上有全屏窗口，无论哪个窗口获得焦点，它都会保持隐藏。\n"
         L"在同一个应用中切换键盘，语言项目也会显示出来。\n"
         L"鼠标沿屏幕底边滑到程序坞时，程序坞会弹出。\n"
         L"点击桌面应用的通知会跳转到通知指向的位置（适用于注册了此功能的应用）。\n"
         L"面板会跟随胶囊的圆角，截图后胶囊也不会再变化。\n"
         L"在程序坞中点击的应用会从点击开始跳动，直到窗口出现。",
         L"#Neu\n"
         L"{Setup}Ein Einrichtungsassistent (Tab „Einrichtungsassistent“, unter "
         L"„Info“) führt durch die wichtigsten Optionen: den Stil, die Elemente "
         L"der Kapsel, das Ausblenden, die Benachrichtigungen und das Aussehen, "
         L"jede live zu sehen und mit „Abbrechen“ rückgängig zu machen; bei "
         L"einer Neuinstallation öffnet er sich von selbst.\n"
         L"{IconPack}Symbolpakete (Tab „Symbole“): eines von 12 Sets "
         L"(Fluent, Lucide, Material, Phosphor, Emoji und mehr, einmal "
         L"heruntergeladen) oder ein Ordner mit eigenen Bildern; jedes "
         L"einzelne Symbol kann auch ein eigenes Bild oder eine eigene Farbe "
         L"bekommen.\n"
         L"{HideWhen}Wähle, wann die Insel zur Linie wird (Tab "
         L"„Ausblenden“): wenn die Maus weg ist, nur solange ein Fenster "
         L"maximiert ist, oder nur solange ein Fenster darüber liegt; auf "
         L"dem Desktop bleibt sie.\n"
         L"{HideClock}Beim Ausblenden nur die Uhr (Tab „Ausblenden“): statt "
         L"der Linie bleibt eine kleine Kapsel mit der Uhrzeit; solange ein "
         L"Fenster darunter liegt, wird sie zur Linie (Stile Insel und "
         L"Fest). Die mit Win+J ausgeblendete Leiste schrumpft zur selben "
         L"Kapsel; die Maus darauf holt die Leiste zurück.\n"
         L"{LineFade}Die Linie kann nach 30 s, 2 min oder 10 min Ruhe "
         L"verblassen (Tab „Ausblenden“), damit sie auf OLED-Bildschirmen "
         L"nicht stehen bleibt; die Maus oder eine Benachrichtigung holt sie "
         L"zurück.\n"
         L"{NotchOpen}Mit Notches wählst du, welche Notch-Linie die "
         L"ausgeblendete Insel öffnet (Tab „Stil“); die anderen Linien "
         L"verschwinden, Klicks dort gehen an die Fenster darunter.\n"
         L"{Notches}Notches auch im Insel-Stil (Tab „Stil“), wie im Stil "
         L"„Fest“, aber ohne einen Bildschirmstreifen zu belegen; jeder Stil "
         L"behält seine eigene Wahl.\n"
         L"{BarSide}Der Leisten-Stil auch am linken oder rechten Rand (Tab "
         L"„Aussehen“), die Elemente untereinander, mit abgerundeten inneren "
         L"Ecken, wo die Leiste auf den Bildschirm trifft.\n"
         L"{BarClear}Eine durchsichtige Leiste (Tab „Aussehen“): nur ihre "
         L"Elemente über dem, was dahinter ist, oder mit dem Milchglas nur "
         L"die Unschärfe.\n"
         L"{TextSize}Die Textgröße hat einen eigenen Regler (Tab "
         L"„Aussehen“), getrennt von der Symbolgröße.\n"
         L"{AiUsage}KI-Nutzung in der Kapsel (Tab „Apps und Tools“): wie "
         L"viel der Limits von Claude Code (im Terminal) und Codex "
         L"verbraucht ist und wann sie neu beginnen, auf diesem PC gelesen, "
         L"ohne Anmeldung und ohne Internet.\n"
         L"{Commands}Deine Befehle (Tab „Apps und Tools“): Verknüpfungen zu "
         L"Programmen, Dateien, Ordnern, Links oder Befehlszeilen mit "
         L"eigenen Symbolen, in einem Bereich; jede kann auch ein Knopf in "
         L"der Kapsel sein.\n"
         L"{FocusedTitle}Die App im Fokus kann den Fenstertitel neben ihrem "
         L"Namen zeigen (Tab „Desktop“).\n"
         L"{CalendarBigDay}Eine Kalenderkarte (Tab „Uhr und Kalender“): der "
         L"Tag groß neben dem Monat, Knöpfe für Wecker, Timer und deine "
         L"Kalender, die Termine des Tages und was als Nächstes kommt.\n"
         L"{WeatherCards}Die Wetterdetails als Karten, wie auf dem Handy "
         L"(Tab „Apps und Tools“, unter Wetter).\n"
         L"{WeatherUnit}Celsius oder Fahrenheit für das Wetter (Tab „Apps "
         L"und Tools“, unter Wetter).\n"
         L"{CapsLock}Eine Blase, wenn die Feststelltaste ein- oder "
         L"ausgeschaltet wird (Tab „Banner“).\n"
         L"{Profiles}Profile, die sich selbst einschalten (Tab „Allgemein“, "
         L"der Stift eines Profils): mit bestimmten Apps im Vordergrund oder "
         L"einem gewählten Stil; nach ihrer App geht es zum vorherigen "
         L"Profil zurück oder bleibt.\n"
         L"Name und Passwort des mobilen Hotspots lassen sich im "
         L"Kontrollzentrum auf der Seite des Hotspots ändern.\n"
         L"{Dock}Das Dock kann auf dem Desktop sichtbar bleiben und sich nur "
         L"ausblenden, solange ein Fenster maximiert ist oder darüber liegt, "
         L"wie die Insel (Windhawk-Einstellungen).\n"
         L"{Dock}Das Dock kann bei einer gewählten Breite enden; die übrigen "
         L"Apps scrollen mit dem Mausrad, wie in einer Box "
         L"(Windhawk-Einstellungen).\n"
         L"{Dock}Vergrößerung, Hüpfen und „Wann sie sich ausblendet“ des "
         L"Docks auch in der Windows-Taskleiste nur mit den Apps (eine "
         L"Option in den Windhawk-Einstellungen).\n"
         L"{BannerPlace}Banner an einer von neun Stellen des Bildschirms, "
         L"auf einem kleinen Bildschirm gewählt (Tab „Banner“), vom nächsten "
         L"Rand hereingleitend; oder wie bisher aus der Kapsel.\n"
         L"{Shelf}Eine Ablage für Dateien (Tab „Desktop“): Dateien irgendwo "
         L"auf die Kapsel ziehen, um sie dort abzulegen (nur ihr Ort, keine "
         L"Kopien), dann ihr Symbol öffnen und sie in eine andere App ziehen.\n"
         L"{FolderThumbnails}Miniaturansichten von Bildern, Videos und "
         L"Dokumenten im Ordnerbereich und in der Ablage (Tab „Desktop“); "
         L"lass die Maus eine halbe Sekunde darauf ruhen für eine größere "
         L"Vorschau.\n"
         L"{TimerLine}Läuft bei minimierter Insel ein Timer, Pomodoro oder "
         L"die Stoppuhr, wird die Linie zu einer kleinen Kapsel mit Ring und "
         L"Zeit (Tab „Ausblenden“); mit „Beim Ausblenden nur die Uhr“ steht "
         L"die Zeit neben der Uhr.\n"
         L"{LineClick}Die ausgeblendete Linie kann sich mit einem Klick "
         L"statt mit der Maus öffnen (Tab „Ausblenden“): unter der Maus wird "
         L"sie nur größer, damit sich die Insel nicht versehentlich öffnet; "
         L"auch die Uhr der ausgeblendeten Leiste.\n"
         L"Das Wetter kann deinen Standort verwenden („Meinen Standort "
         L"verwenden“ im Wetterbereich), mit der eigenen Erlaubnis von "
         L"Windows; er geht auf etwa einen Kilometer gerundet an den "
         L"Wetterdienst, und eine eingegebene Stadt schaltet zurück.\n"
         L"Wecker können einen Namen und die Tage haben, an denen sie "
         L"klingeln: jeden Tag, werktags, am Wochenende, beliebige Tage oder "
         L"einmal (danach schaltet er sich selbst aus).\n"
         L"{GlassLight}Licht und Schatten wie in macOS 27 für das Liquid "
         L"Glass (Tab „Aussehen“): Glanz, abgeschrägter Rand, Licht am "
         L"unteren Rand und ein weicher Schatten, und die Karten der "
         L"Bereiche wirken ebenfalls wie Glas. Eine Idee von Thiago Silva.\n"
         L"Die Insel spricht Vietnamesisch (Tiếng Việt), übersetzt von caotu2k5.\n"
         L"#Verbesserungen\n"
         L"Das Einstellungsfenster sieht aus wie bei Windows 11: eigene "
         L"Titelleiste, größere Texte, kleine Optionen in einem breiten "
         L"Fenster zu zweit pro Zeile und immer sichtbare Bildlaufleisten, die "
         L"sich ziehen lassen (auch die der Seitenleiste).\n"
         L"Die Blasen, die etwas fragen oder sagen (was eine Option braucht, "
         L"die Tipps der Symbole, die Einstellungsblase), sind deckend, egal "
         L"welche Deckkraft die Insel hat.\n"
         L"Die Website des Mods, island.caliberda.com.br (Tab Info): jede "
         L"Option erklärt, die Roadmap und deine Stimmen.\n"
         L"{Style}Einstellungen neu geordnet: neue Tabs Stil, Ausblenden, "
         L"Kontrollzentrum und Banner, voneinander abhängige Optionen "
         L"beisammen.\n"
         L"Eine ausgegraute Option sagt warum; ein Klick auf diese Zeile "
         L"zeigt alles, was sie braucht, „Ja, ändern“ schaltet alles auf "
         L"einmal ein und „Änderungen zurücksetzen“ macht es rückgängig.\n"
         L"Auswahlen schneiden ihre Namen nie ab (sie gehen bei Bedarf in "
         L"mehr Zeilen), und Orte und Ecken werden auf Zeichnungen gewählt.\n"
         L"In „Neuigkeiten“ leuchtet eine Funktion mit Pfeil unter der Maus "
         L"auf, und ein Klick darauf zeigt ihre Option.\n"
         L"{FolderDrag}Aus dem Ordnerbereich oder der Ablage gezogene "
         L"Dateien: beim ersten Mal gefragt (dann gemerkt), jedes Mal "
         L"gefragt, kopiert oder verschoben (Tab „Desktop“).\n"
         L"{MinimizeKey}Win+J (Tab „Ausblenden“) blendet auch im "
         L"Leisten-Stil die ganze Leiste aus und holt sie zurück.\n"
         L"{ExternalBrightness}Die Helligkeit externer Monitore über ihr "
         L"Kabel (DDC/CI) lässt sich ausschalten (Tab „Kontrollzentrum“), "
         L"für Monitore, die flackern, oder Docks und MST-Hubs, die beim "
         L"Öffnen des Kontrollzentrums neu starten.\n"
         L"{BannerSize}Größe (50 % bis 200 %), Breite und Ecken des Banners "
         L"sind wählbar (Tab „Banner“).\n"
         L"{DndSilent}Anrufe, Wecker und Erinnerungen können im „Nicht "
         L"stören“ ebenfalls stumm bleiben (Tab „Benachrichtigungen“).\n"
         L"{DownloadStalled}Ein Browser-Download, der 30 s stillsteht, "
         L"erscheint pausiert, in Gelb (Tab „System“): Browser melden nie, "
         L"dass ein Download pausiert ist.\n"
         L"{Reset}Ein Zurücksetzen-Knopf neben Exportieren und Importieren "
         L"(Tab „Allgemein“, Doppelklick) stellt die Einstellungen der Insel "
         L"wieder her; Notizen, Aufgaben, Termine und Profile bleiben.\n"
         L"Der Mixer des Kontrollzentrums zeigt die Lautstärke jeder App in "
         L"Prozent.\n"
         L"Direkt aus der Kapsel geöffnete Seiten haben einen Zurück-Pfeil.\n"
         L"Der Termineditor sagt, was seine Erinnerung tun wird.\n"
         L"Die Kalenderkarte nennt den Monat ihrer Tage, über ihnen.\n"
         L"#Korrekturen\n"
         L"Das Dock behält die Symbole, die für die an die Taskleiste "
         L"angehefteten Verknüpfungen festgelegt sind, statt des eigenen "
         L"Symbols der App (#6891).\n"
         L"Mit einem Monitor anderer Skalierung darunter wird das Dock nach "
         L"dem Wechsel des Taskleistenmodus nicht mehr außermittig "
         L"abgeschnitten.\n"
         L"Alt oder F10 im Einstellungsfenster oder sein Verschieben halten "
         L"die Animationen der Inseln nicht mehr an.\n"
         L"Die Infobereichssymbole von Programmen, die die Insel nicht öffnen "
         L"kann (wie das von NVIDIA), behalten ihr Bild, nachdem der Mod aus- "
         L"und eingeschaltet wurde.\n"
         L"Der Wetterbereich auf der Insel eines anderen Monitors bleibt nicht "
         L"mehr bei „Suche“ hängen.\n"
         L"„Änderungen zurücksetzen“ bleibt nach einem Neustart der Inseln "
         L"erhalten und lässt eine seitdem wieder geänderte Option in Ruhe.\n"
         L"Die Helligkeit, die Feststelltaste mit drei Notches, die Anzahl der "
         L"Ablage und „Fenster darüber“ bei den Docks anderer Monitore "
         L"funktionieren wie vorgesehen.\n"
         L"Mit Windhawk als Administrator erscheinen die Infobereich-Symbole "
         L"der anderen Apps, ihre Zähler und der Download-Fortschritt wieder "
         L"auf der Insel.\n"
         L"Mit ausgeschalteten Animationen öffnen sich Bereiche ohne "
         L"Flackern der Kapsel, und die Bestätigung des Ein/Aus-Knopfs "
         L"erscheint.\n"
         L"Verknüpfungen und Notizen behalten ihre Zeilenumbrüche in "
         L"Windhawk portable.\n"
         L"Liedtexte für Apple Music (die App, Cider und die Webseite) und "
         L"für Player, die die Länge eines Lieds nicht angeben.\n"
         L"Apps mit Benachrichtigungen, die auf einer Insel (einer Notch "
         L"oder der eines anderen Monitors) gesehen wurden, gelten auf allen "
         L"als gesehen.\n"
         L"Kein weißer Streifen mehr unter dem Dock oder der durchsichtigen "
         L"Taskleiste, nachdem sich die Taskleiste selbst ausgeblendet hat.\n"
         L"Die Insel bleibt nicht mehr leer bei Grafiktreibern, die Bilder "
         L"nicht weich verkleinern können.\n"
         L"Im Leisten-Stil mit der Uhr in der Mitte blinken Elemente nicht "
         L"mehr über der Uhr.\n"
         L"Der Zähler des Papierkorbs bleibt aktuell, wenn der Explorer nach "
         L"der Insel startet.\n"
         L"Das Ändern des Energiemodus im Kontrollzentrum lässt die Insel "
         L"nicht mehr hängen.\n"
         L"Die an eine Seite verschobene Leiste gibt den oberen "
         L"Bildschirmrand frei (und die Seite, wenn sie wieder wechselt).\n"
         L"Auf einem HDR-Monitor ändert sein Helligkeitsregler die "
         L"Helligkeit des Desktops (die Helligkeit von SDR-Inhalten in "
         L"Windows).\n"
         L"Orte und Symbole pro App kehren auf dem Desktop und in der "
         L"Taskleiste zu den allgemeinen zurück, statt die der letzten App "
         L"zu bleiben.\n"
         L"Mit einem weiteren Monitor darunter gleitet das ausgeblendete "
         L"Dock über den Bildschirmrand hinaus, statt zu einer Linie darüber "
         L"zu schrumpfen.\n"
         L"Das Element 中/英 folgt Shift auch bei Eingabemethoden, die sonst "
         L"nichts melden, wie dem neuesten Microsoft Pinyin.\n"
         L"Eigene Symbole einer App, gewählt bevor es manche Elemente gab, "
         L"zeigen diese nicht mehr, solange sie aus sind.\n"
         L"Die Auslastungsseite des Kontrollzentrums zeigt CPU, RAM und "
         L"VRAM auch ohne diese Elemente in der Kapsel, mit einem Tab für "
         L"jedes.\n"
         L"Mit zwei Monitoren bleibt eine Insel verborgen, solange ihr "
         L"eigener Monitor ein Vollbildfenster hat, egal welches Fenster "
         L"den Fokus hat.\n"
         L"Eine in derselben App gewechselte Tastatur erscheint im "
         L"Sprachelement.\n"
         L"Das Dock erscheint, wenn die Maus am unteren Rand entlang zu "
         L"ihm gleitet.\n"
         L"Ein Klick auf die Mitteilung einer Desktop-App führt dorthin, "
         L"wohin sie zeigt, bei Apps, die das registrieren.\n"
         L"Die Bereiche folgen den Ecken der Kapsel, und die Kapsel "
         L"ändert sich nach einem Screenshot nicht mehr.\n"
         L"Eine im Dock angeklickte App hüpft vom Klick an, bis ihr "
         L"Fenster erscheint.",
         L"#Novedades\n"
         L"{Setup}Un asistente de configuración (pestaña Asistente de "
         L"configuración, debajo de Acerca de) recorre las opciones "
         L"principales: el estilo, los elementos de la píldora, la ocultación, "
         L"las notificaciones y el aspecto, cada una en vivo y deshecha con "
         L"Cancelar; se abre solo en una instalación nueva.\n"
         L"{IconPack}Paquetes de iconos (pestaña Iconos): uno de 12 "
         L"conjuntos (Fluent, Lucide, Material, Phosphor, Emoji y más, "
         L"descargados una vez) o una carpeta con tus imágenes; cualquier "
         L"icono también puede tener su propia imagen o color.\n"
         L"{HideWhen}Elige cuándo la isla se convierte en la línea (pestaña "
         L"Ocultar): con el ratón lejos, solo mientras una ventana está "
         L"maximizada, o solo mientras una ventana está encima; en el "
         L"escritorio se queda.\n"
         L"{HideClock}Solo el reloj al ocultarse (pestaña Ocultar): en lugar "
         L"de la línea queda una píldora pequeña con la hora; se convierte "
         L"en la línea mientras una ventana está debajo (estilos Isla y "
         L"Fija). La Barra ocultada con Win+J se cierra en la misma píldora; "
         L"el ratón encima trae la barra de vuelta.\n"
         L"{LineFade}La línea puede desvanecerse tras 30 s, 2 min o 10 min "
         L"sin uso (pestaña Ocultar), para que no quede marcada en pantallas "
         L"OLED; el ratón o una notificación la traen de vuelta.\n"
         L"{NotchOpen}Con muescas, elige qué línea de muesca abre la isla "
         L"oculta (pestaña Estilo); las otras líneas se ocultan y los clics "
         L"allí van a las ventanas de abajo.\n"
         L"{Notches}Muescas también en el estilo Isla (pestaña Estilo), como "
         L"las de la Fija pero sin reservar una franja de la pantalla; cada "
         L"estilo guarda su elección.\n"
         L"{BarSide}El estilo Barra también en el borde izquierdo o derecho "
         L"(pestaña Aspecto), con los elementos uno debajo del otro y las "
         L"esquinas interiores redondeadas donde se une a la pantalla.\n"
         L"{BarClear}Una barra transparente (pestaña Aspecto): solo sus "
         L"elementos sobre lo que hay detrás, o, con el vidrio esmerilado, "
         L"solo el desenfoque.\n"
         L"{TextSize}El tamaño del texto tiene su propio control (pestaña "
         L"Aspecto), aparte del de los iconos.\n"
         L"{AiUsage}Uso de IA en la píldora (pestaña Apps y herramientas): "
         L"cuánto de los límites de Claude Code (en un terminal) y Codex se "
         L"ha usado y cuándo se reinician, leído en este PC, sin inicio de "
         L"sesión y sin nada en línea.\n"
         L"{Commands}Tus comandos (pestaña Apps y herramientas): accesos a "
         L"programas, archivos, carpetas, enlaces o líneas de comando, con "
         L"sus propios iconos, en un panel; cualquiera puede ser también un "
         L"botón en la píldora.\n"
         L"{FocusedTitle}La app en foco puede mostrar el título de su "
         L"ventana junto a su nombre (pestaña Escritorio).\n"
         L"{CalendarBigDay}Una tarjeta del calendario (pestaña Reloj y "
         L"calendario): el día grande junto al mes, botones para alarma, "
         L"temporizador y tus agendas, los eventos del día y lo que viene.\n"
         L"{WeatherCards}Los detalles del tiempo en tarjetas, como en el "
         L"teléfono (pestaña Apps y herramientas, en Tiempo).\n"
         L"{WeatherUnit}Celsius o Fahrenheit para el tiempo (pestaña Apps y "
         L"herramientas, en Tiempo).\n"
         L"{CapsLock}Una burbuja que avisa cuando Bloq Mayús se activa o "
         L"desactiva (pestaña Banners).\n"
         L"{Profiles}Perfiles que se activan solos (pestaña General, el "
         L"lápiz de un perfil): con algunas apps en primer plano o con un "
         L"estilo elegido; después de su app, vuelve al perfil anterior o se "
         L"queda.\n"
         L"El nombre y la contraseña del punto de acceso móvil se pueden "
         L"cambiar en el centro de control, en la página del punto de acceso.\n"
         L"{Dock}El dock puede quedarse visible en el escritorio y ocultarse "
         L"solo mientras una ventana está maximizada o encima, como la isla "
         L"(configuración de Windhawk).\n"
         L"{Dock}El dock puede quedarse en un ancho que elijas; las demás "
         L"apps se desplazan con la rueda del ratón, como una caja "
         L"(configuración de Windhawk).\n"
         L"{Dock}La ampliación, los rebotes y el \"Cuándo se oculta\" del "
         L"dock también en la barra de tareas de Windows mantenida solo con "
         L"las apps (una opción en la configuración de Windhawk).\n"
         L"{BannerPlace}Banners en uno de nueve puntos de la pantalla, "
         L"elegido en una pantallita (pestaña Banners), entrando desde el "
         L"borde más cercano; o saliendo de la píldora, como antes.\n"
         L"{Shelf}Un estante de archivos (pestaña Escritorio): suelta "
         L"archivos en cualquier lugar de la píldora para guardarlos allí "
         L"(solo su ubicación, no copias), luego abre su icono y arrástralos "
         L"a otra app.\n"
         L"{FolderThumbnails}Miniaturas de imágenes, videos y documentos en "
         L"el panel de carpetas y en el estante (pestaña Escritorio); deja "
         L"el ratón medio segundo sobre una para ver una vista previa más "
         L"grande.\n"
         L"{TimerLine}Mientras corre un temporizador, el Pomodoro o el "
         L"cronómetro con la isla minimizada, la línea se vuelve una píldora "
         L"pequeña con su anillo y su tiempo (pestaña Ocultar); con «Solo el "
         L"reloj al ocultarse», el tiempo va junto al reloj.\n"
         L"{LineClick}La línea oculta puede abrirse con un clic en lugar del "
         L"ratón (pestaña Ocultar): bajo el ratón solo crece, para que la "
         L"isla no se abra sin querer; también el reloj de la Barra oculta.\n"
         L"El clima puede usar tu ubicación («Usar mi ubicación» en el panel "
         L"del clima), con el propio permiso de Windows; va al servicio del "
         L"clima redondeada a cerca de un kilómetro, y escribir una ciudad "
         L"vuelve a ella.\n"
         L"Las alarmas pueden tener nombre y los días en que suenan: todos "
         L"los días, entre semana, fines de semana, cualquier día o una vez "
         L"(se apaga sola después de sonar).\n"
         L"{GlassLight}Luz y sombra como en macOS 27 para el Liquid Glass "
         L"(pestaña Aspecto): brillo, borde biselado, luz en su parte "
         L"inferior y una sombra suave, y las tarjetas de los paneles "
         L"también parecen de vidrio. Una idea de Thiago Silva.\n"
         L"La isla habla vietnamita (Tiếng Việt), traducido por caotu2k5.\n"
         L"#Mejoras\n"
         L"La ventana de configuración se ve como la de Windows 11: barra de "
         L"título propia, textos más grandes, las opciones pequeñas de dos en "
         L"dos en una ventana ancha y barras de desplazamiento siempre "
         L"visibles que se pueden arrastrar (también la de la barra lateral).\n"
         L"Los globos que preguntan o avisan algo (lo que necesita una opción, "
         L"las sugerencias de los iconos, el globo de configuración) son "
         L"opacos, sea cual sea la opacidad de la isla.\n"
         L"El sitio del mod, island.caliberda.com.br (pestaña Acerca de): cada "
         L"opción explicada, la hoja de ruta y tus votos.\n"
         L"{Style}Configuración reorganizada: pestañas nuevas Estilo, "
         L"Ocultar, Centro de control y Banners, con las opciones que "
         L"dependen entre sí juntas.\n"
         L"Una opción atenuada dice por qué; un clic en esa línea muestra "
         L"todo lo que necesita, \"Sí, cambiar\" lo activa todo de una vez y "
         L"\"Restaurar lo que cambió\" lo deshace.\n"
         L"Las opciones nunca cortan sus nombres (pasan a más filas si hace "
         L"falta), y los lugares y las esquinas se eligen en dibujos.\n"
         L"En Novedades, una función con flecha se ilumina bajo el ratón, y "
         L"un clic en ella muestra su opción.\n"
         L"{FolderDrag}Archivos arrastrados fuera del panel de carpetas o "
         L"del estante: se pregunta la primera vez (y se recuerda), se "
         L"pregunta siempre, se copian o se mueven (pestaña Escritorio).\n"
         L"{MinimizeKey}Win+J (pestaña Ocultar) oculta toda la barra también "
         L"en el estilo Barra, y la trae de vuelta.\n"
         L"{ExternalBrightness}El brillo de los monitores externos por su "
         L"cable (DDC/CI) se puede desactivar (pestaña Centro de control), "
         L"para monitores que parpadean o docks y hubs MST que se reinician "
         L"al abrir el centro de control.\n"
         L"{BannerSize}Se pueden elegir el tamaño (50 % a 200 %), el ancho y "
         L"las esquinas del banner (pestaña Banners).\n"
         L"{DndSilent}Las llamadas, alarmas y recordatorios también pueden "
         L"quedarse en silencio en No molestar (pestaña Notificaciones).\n"
         L"{DownloadStalled}Una descarga del navegador detenida durante 30 s "
         L"se ve en pausa, en amarillo (pestaña Sistema): los navegadores "
         L"nunca avisan que una descarga está en pausa.\n"
         L"{Reset}Un botón Restablecer junto a Exportar e Importar (pestaña "
         L"General, doble clic) devuelve la configuración de la isla a la "
         L"original; notas, tareas, eventos y perfiles se quedan.\n"
         L"El mezclador del centro de control muestra el volumen de cada app "
         L"en porcentaje.\n"
         L"Las páginas abiertas directamente desde la píldora tienen una "
         L"flecha para volver.\n"
         L"El editor de eventos dice qué hará su recordatorio.\n"
         L"La tarjeta del calendario nombra el mes de sus días, encima de "
         L"ellos.\n"
         L"#Correcciones\n"
         L"El dock conserva los iconos puestos en los accesos directos "
         L"anclados a la barra de tareas, en lugar del icono propio de la app "
         L"(#6891).\n"
         L"Con un monitor de otra escala debajo, el dock ya no queda cortado y "
         L"descentrado tras cambiar el modo de la barra de tareas.\n"
         L"Alt o F10 en la ventana de configuración, o moverla, ya no detienen "
         L"las animaciones de las islas.\n"
         L"Los iconos de la bandeja de programas que la isla no puede abrir "
         L"(como el de NVIDIA) conservan su imagen tras apagar y encender el "
         L"mod.\n"
         L"El panel del tiempo en la isla de otro monitor ya no se queda en "
         L"Buscando.\n"
         L"\"Restaurar lo que cambió\" sigue tras reiniciarse las islas y no "
         L"toca una opción cambiada de nuevo desde entonces.\n"
         L"El brillo, Bloq Mayús con tres muescas, el número del estante y "
         L"\"Ventana encima\" en los docks de otros monitores funcionan como "
         L"deben.\n"
         L"Con Windhawk como administrador, los iconos de la bandeja de las "
         L"otras apps, sus contadores y el progreso de las descargas vuelven "
         L"a aparecer en la isla.\n"
         L"Con las animaciones desactivadas, los paneles se abren sin que la "
         L"píldora parpadee, y aparece la confirmación del botón de apagado.\n"
         L"Los accesos y las notas mantienen sus saltos de línea en Windhawk "
         L"portable.\n"
         L"Letras de canciones para Apple Music (la app, Cider y su página "
         L"web) y para reproductores que no indican la duración de la canción.\n"
         L"Las apps con notificaciones vistas en una isla (una muesca o la "
         L"de otro monitor) quedan vistas en todas.\n"
         L"Ya no aparece una franja blanca bajo el dock o la barra de tareas "
         L"transparente después de que la barra se oculta sola.\n"
         L"La isla ya no se queda vacía con controladores gráficos que no "
         L"pueden reducir imágenes de forma suave.\n"
         L"En el estilo Barra con el reloj en el centro, los elementos ya no "
         L"parpadean sobre el reloj.\n"
         L"El contador de la Papelera se mantiene al día cuando el "
         L"Explorador arranca después de la isla.\n"
         L"Cambiar el modo de energía en el centro de control ya no congela "
         L"la isla.\n"
         L"La barra llevada a un lado devuelve la parte superior de la "
         L"pantalla (y el lado, cuando cambia otra vez).\n"
         L"En un monitor HDR, su control de brillo cambia el brillo del "
         L"escritorio (el brillo del contenido SDR de Windows).\n"
         L"Los lugares e iconos por app vuelven a los generales en el "
         L"escritorio y en la barra de tareas, en lugar de quedarse los de "
         L"la última app.\n"
         L"Con otro monitor debajo, el dock oculto se desliza fuera por el "
         L"borde de la pantalla, en lugar de encogerse en una línea encima.\n"
         L"El elemento 中/英 sigue a Shift con métodos de entrada que no "
         L"avisan nada más, como el Microsoft Pinyin más nuevo.\n"
         L"Los iconos propios de una app, elegidos antes de que existieran "
         L"algunos elementos, ya no muestran esos elementos mientras están "
         L"desactivados.\n"
         L"La página de uso del centro de control muestra CPU, RAM y VRAM "
         L"sin esos elementos en la píldora, con una pestaña para cada "
         L"uno.\n"
         L"Con dos monitores, una isla sigue oculta mientras su propio "
         L"monitor tiene una ventana a pantalla completa, sea cual sea la "
         L"ventana con el foco.\n"
         L"Un teclado cambiado en la misma app se muestra en el elemento "
         L"del idioma.\n"
         L"El dock aparece cuando el ratón se desliza por el borde "
         L"inferior hasta él.\n"
         L"Un clic en la notificación de una app de escritorio va adonde "
         L"apunta, en las apps que lo registran.\n"
         L"Los paneles siguen las esquinas de la píldora, y la píldora ya "
         L"no cambia después de una captura de pantalla.\n"
         L"Una app pulsada en el dock rebota desde el clic hasta que "
         L"aparece su ventana.",
         L"#Новое\n"
         L"{Setup}Мастер настройки (вкладка «Мастер настройки», под «О "
         L"программе») проведёт по основным параметрам: стиль, элементы "
         L"капсулы, скрытие, уведомления и вид; каждый виден сразу, а "
         L"«Отмена» всё возвращает; при новой установке он открывается сам.\n"
         L"{IconPack}Наборы значков (вкладка «Значки»): один из 12 наборов "
         L"(Fluent, Lucide, Material, Phosphor, Emoji и другие, загружаются "
         L"один раз) или папка с вашими картинками; любому значку можно дать "
         L"свою картинку или цвет.\n"
         L"{HideWhen}Выберите, когда остров превращается в линию (вкладка "
         L"«Скрытие»): когда мышь далеко, только пока окно развёрнуто или "
         L"только пока окно над ним; на рабочем столе он остаётся.\n"
         L"{HideClock}Только часы при скрытии (вкладка «Скрытие»): вместо "
         L"линии остаётся маленькая капсула со временем; пока под ней окно, "
         L"она становится линией (стили «Остров» и «Закреплённый»). Панель, "
         L"скрытая через Win+J, сжимается в такую же капсулу; мышь на ней "
         L"возвращает панель.\n"
         L"{LineFade}Линия может исчезать после 30 с, 2 мин или 10 мин покоя "
         L"(вкладка «Скрытие»), чтобы не выгорать на OLED-экранах; мышь или "
         L"уведомление возвращают её.\n"
         L"{NotchOpen}С вырезами выберите, линия какого выреза открывает "
         L"скрытый остров (вкладка «Стиль»); остальные линии скрыты, а "
         L"щелчки там идут окнам под ними.\n"
         L"{Notches}Вырезы и в стиле «Остров» (вкладка «Стиль»), как в "
         L"закреплённом, но без резервирования полосы экрана; каждый стиль "
         L"хранит свой выбор.\n"
         L"{BarSide}Стиль «Панель» также у левого или правого края (вкладка "
         L"«Вид»), элементы друг под другом, со скруглёнными внутренними "
         L"углами там, где она встречает экран.\n"
         L"{BarClear}Прозрачная панель (вкладка «Вид»): только её элементы "
         L"поверх того, что позади, или с матовым стеклом только размытие.\n"
         L"{TextSize}У размера текста свой ползунок (вкладка «Вид»), "
         L"отдельно от размера значков.\n"
         L"{AiUsage}Использование ИИ в капсуле (вкладка «Приложения и "
         L"инструменты»): сколько лимитов Claude Code (в терминале) и Codex "
         L"израсходовано и когда они обновятся, читается на этом ПК, без "
         L"входа и без интернета.\n"
         L"{Commands}Ваши команды (вкладка «Приложения и инструменты»): "
         L"ярлыки программ, файлов, папок, ссылок или командных строк со "
         L"своими значками, на панели; любой из них может стать и кнопкой в "
         L"капсуле.\n"
         L"{FocusedTitle}Приложение в фокусе может показывать заголовок окна "
         L"рядом с названием (вкладка «Рабочий стол»).\n"
         L"{CalendarBigDay}Карточка календаря (вкладка «Часы и календарь»): "
         L"крупный день рядом с месяцем, кнопки будильника, таймера и ваших "
         L"календарей, события дня и то, что впереди.\n"
         L"{WeatherCards}Подробности погоды карточками, как на телефоне "
         L"(вкладка «Приложения и инструменты», в разделе «Погода»).\n"
         L"{WeatherUnit}Цельсий или Фаренгейт для погоды (вкладка "
         L"«Приложения и инструменты», в разделе «Погода»).\n"
         L"{CapsLock}Пузырёк, когда Caps Lock включается или выключается "
         L"(вкладка «Баннеры»).\n"
         L"{Profiles}Профили, которые включаются сами (вкладка «Общие», "
         L"карандаш профиля): при некоторых приложениях на переднем плане "
         L"или выбранном стиле; после приложения возвращается прежний "
         L"профиль или остаётся этот.\n"
         L"Имя и пароль мобильной точки доступа можно изменить в центре "
         L"управления, на её странице.\n"
         L"{Dock}Док может оставаться на рабочем столе и скрываться, только "
         L"пока окно развёрнуто или над ним, как остров (настройки Windhawk).\n"
         L"{Dock}Док может останавливаться на выбранной ширине; остальные "
         L"приложения прокручиваются колесом мыши, как в коробке (настройки "
         L"Windhawk).\n"
         L"{Dock}Увеличение, подпрыгивание и «Когда скрывать» дока также на "
         L"панели задач Windows, оставленной только с приложениями (параметр "
         L"в настройках Windhawk).\n"
         L"{BannerPlace}Баннеры в одной из девяти точек экрана, выбранной на "
         L"маленьком экране (вкладка «Баннеры»), выезжают с ближайшего края; "
         L"или из капсулы, как раньше.\n"
         L"{Shelf}Полка для файлов (вкладка «Рабочий стол»): бросьте файлы в "
         L"любое место капсулы, чтобы сохранить их там (только их "
         L"расположение, без копий), затем откройте её значок и перетащите "
         L"файлы в другое приложение.\n"
         L"{FolderThumbnails}Миниатюры изображений, видео и документов на "
         L"панели папок и на полке (вкладка «Рабочий стол»); задержите мышь "
         L"на полсекунды для крупного предпросмотра.\n"
         L"{TimerLine}Пока при свёрнутом острове идёт таймер, помидор или "
         L"секундомер, линия становится маленькой капсулой с кольцом и "
         L"временем (вкладка «Скрытие»); с «Только часы при скрытии» время "
         L"показано рядом с часами.\n"
         L"{LineClick}Скрытая линия может открываться щелчком, а не "
         L"наведением мыши (вкладка «Скрытие»): под мышью она только "
         L"увеличивается, чтобы остров не открывался случайно; так же и часы "
         L"скрытой панели.\n"
         L"Погода может использовать ваше расположение («Использовать моё "
         L"расположение» на панели погоды) с разрешения самой Windows; оно "
         L"отправляется в службу погоды, округлённое примерно до километра, "
         L"а ввод города возвращает к нему.\n"
         L"У будильников могут быть название и дни звонка: каждый день, по "
         L"будням, по выходным, любые дни или один раз (после звонка "
         L"выключается сам).\n"
         L"{GlassLight}Свет и тень как в macOS 27 для Liquid Glass (вкладка "
         L"«Вид»): блик, скошенный край, свет снизу и мягкая тень, а "
         L"карточки панелей тоже выглядят как стекло. Идея Thiago Silva.\n"
         L"Остров говорит по-вьетнамски (Tiếng Việt), перевод caotu2k5.\n"
         L"#Улучшения\n"
         L"Окно настроек выглядит как в Windows 11: свой заголовок, текст "
         L"крупнее, небольшие настройки по две в строке в широком окне и "
         L"всегда видимые полосы прокрутки, которые можно перетаскивать (и у "
         L"боковой панели).\n"
         L"Пузыри, которые что-то спрашивают или сообщают (что нужно "
         L"настройке, подсказки значков, пузырь настроек), непрозрачны при "
         L"любой прозрачности острова.\n"
         L"Сайт мода, island.caliberda.com.br (вкладка «О программе»): каждая "
         L"настройка с объяснением, планы и ваши голоса.\n"
         L"{Style}Настройки перестроены: новые вкладки «Стиль», «Скрытие», "
         L"«Центр управления» и «Баннеры», зависимые параметры рядом.\n"
         L"Неактивный параметр объясняет почему; щелчок по этой строке "
         L"показывает всё, что ему нужно, «Да, изменить» включает всё сразу, "
         L"а «Вернуть изменённое» отменяет.\n"
         L"Варианты выбора больше не обрезают названия (при необходимости "
         L"переносятся в несколько строк), а места и углы выбираются на "
         L"рисунках.\n"
         L"В «Что нового» функция со стрелкой подсвечивается под мышью, а "
         L"щелчок по ней показывает её параметр.\n"
         L"{FolderDrag}Файлы, перетаскиваемые с панели папок или с полки: "
         L"спросить в первый раз (и запомнить), спрашивать каждый раз, "
         L"копировать или перемещать (вкладка «Рабочий стол»).\n"
         L"{MinimizeKey}Win+J (вкладка «Скрытие») скрывает всю панель и в "
         L"стиле «Панель», и возвращает её.\n"
         L"{ExternalBrightness}Яркость внешних мониторов по кабелю (DDC/CI) "
         L"можно выключить (вкладка «Центр управления») — для мониторов, "
         L"которые мерцают, и док-станций и MST-хабов, которые "
         L"перезапускаются при открытии центра управления.\n"
         L"{BannerSize}Можно выбрать размер (от 50 до 200 %), ширину и углы "
         L"баннера (вкладка «Баннеры»).\n"
         L"{DndSilent}Звонки, будильники и напоминания тоже могут молчать в "
         L"режиме «Не беспокоить» (вкладка «Уведомления»).\n"
         L"{DownloadStalled}Загрузка браузера, стоящая на месте 30 с, "
         L"показывается приостановленной, жёлтым (вкладка «Система»): "
         L"браузеры никогда не сообщают о паузе.\n"
         L"{Reset}Кнопка «Сбросить» рядом с «Экспорт» и «Импорт» (вкладка "
         L"«Общие», двойной щелчок) возвращает настройки острова по "
         L"умолчанию; заметки, задачи, события и профили остаются.\n"
         L"Микшер центра управления показывает громкость каждого приложения "
         L"в процентах.\n"
         L"У страниц, открытых прямо из капсулы, есть стрелка назад.\n"
         L"Редактор событий объясняет, что сделает его напоминание.\n"
         L"Карточка календаря называет месяц своих дней над ними.\n"
         L"#Исправления\n"
         L"Док сохраняет значки, заданные у ярлыков, закреплённых на панели "
         L"задач, вместо собственного значка приложения (#6891).\n"
         L"С монитором другого масштаба снизу док больше не обрезается не по "
         L"центру после смены режима панели задач.\n"
         L"Alt или F10 в окне настроек, а также его перемещение больше не "
         L"останавливают анимации островов.\n"
         L"Значки в трее у программ, которые остров не может открыть "
         L"(например, NVIDIA), сохраняют картинку после выключения и включения "
         L"мода.\n"
         L"Панель погоды на острове другого монитора больше не зависает на "
         L"«Поиск».\n"
         L"«Вернуть изменённое» остаётся после перезапуска островов и не "
         L"трогает настройку, которую снова изменили после этого.\n"
         L"Яркость, Caps Lock с тремя вырезами, счётчик полки и «Окно поверх» "
         L"у доков других мониторов работают как надо.\n"
         L"С Windhawk от имени администратора значки других приложений в "
         L"области уведомлений, их счётчики и ход загрузок снова видны на "
         L"острове.\n"
         L"С выключенными анимациями панели открываются без мерцания "
         L"капсулы, и появляется подтверждение кнопки питания.\n"
         L"Ярлыки и заметки сохраняют переносы строк в портативном Windhawk.\n"
         L"Тексты песен для Apple Music (приложение, Cider и веб-страница) и "
         L"для плееров, которые не сообщают длительность песни.\n"
         L"Приложения с уведомлениями, просмотренными на одном острове "
         L"(вырезе или острове другого монитора), считаются просмотренными "
         L"на всех.\n"
         L"Больше нет белой полосы под доком или прозрачной панелью задач "
         L"после того, как панель скрывается сама.\n"
         L"Остров больше не остаётся пустым с графическими драйверами, "
         L"которые не умеют плавно уменьшать изображения.\n"
         L"В стиле «Панель» с часами посередине элементы больше не мерцают "
         L"поверх часов.\n"
         L"Счётчик корзины обновляется, когда Проводник запускается после "
         L"острова.\n"
         L"Смена режима питания в центре управления больше не подвешивает "
         L"остров.\n"
         L"Панель, перенесённая на сторону, освобождает верх экрана (и "
         L"сторону, когда переносится снова).\n"
         L"На HDR-мониторе его ползунок яркости меняет яркость рабочего "
         L"стола (яркость SDR-содержимого в Windows).\n"
         L"Расположение и значки для приложений возвращаются к общим на "
         L"рабочем столе и на панели задач, а не остаются от последнего "
         L"приложения.\n"
         L"Если ниже есть другой монитор, скрытый док уезжает за край "
         L"экрана, а не сжимается в линию над ним.\n"
         L"Элемент 中/英 следует за Shift и у методов ввода, которые больше "
         L"ничего не сообщают, как новейший Microsoft Pinyin.\n"
         L"Собственные значки приложения, выбранные до появления некоторых "
         L"элементов, больше не показывают эти элементы, пока они выключены.\n"
         L"Страница загрузки в центре управления показывает ЦП, ОЗУ и "
         L"видеопамять без этих элементов в капсуле, с вкладкой для "
         L"каждого.\n"
         L"С двумя мониторами остров остаётся скрытым, пока на его "
         L"мониторе есть полноэкранное окно, какое бы окно ни было в "
         L"фокусе.\n"
         L"Раскладка, переключённая в том же приложении, показывается в "
         L"элементе языка.\n"
         L"Док появляется, когда мышь скользит к нему вдоль нижнего края.\n"
         L"Щелчок по уведомлению настольного приложения ведёт туда, куда "
         L"оно указывает, в приложениях, которые это регистрируют.\n"
         L"Панели следуют углам капсулы, а капсула больше не меняется "
         L"после снимка экрана.\n"
         L"Приложение, нажатое в доке, подпрыгивает с момента щелчка, "
         L"пока не появится его окно.",
         L"#新機能\n"
         L"{Setup}セットアップ アシスタント（「セットアップ アシスタント」タブ、「バージョン情報」の下）が、主な設定（スタイル、"
         L"カプセルの項目、非表示、通知、外観）を順に案内します。変更はその場で反映され、「キャンセル」で元に戻せます。"
         L"新規インストールでは自動的に開きます。\n"
         L"{IconPack}アイコン パック（「アイコン」タブ）：12 "
         L"のセット（Fluent、Lucide、Material、Phosphor、Emoji "
         L"など、一度だけダウンロード）または自分の画像フォルダー。どのアイコンにも個別の画像や色を付けられます。\n"
         L"{HideWhen}アイランドがラインになるタイミングを選べます（「非表示」タブ）：マウスが離れたとき、ウィンドウが最大化されている"
         L"間だけ、またはウィンドウが重なっている間だけ。デスクトップでは表示されたままです。\n"
         L"{HideClock}隠すときは時計だけ（「非表示」タブ）：ラインの代わりに時刻入りの小さなカプセルが残ります。下にウィンドウがある"
         L"間はラインになります（アイランドと固定スタイル）。Win+J で隠したバーも同じカプセルになり、マウスを乗せるとバーが戻ります。\n"
         L"{LineFade}ラインは 30 秒、2 分、10 分そのままにすると消えます（「非表示」タブ）。OLED "
         L"画面に残らないためで、マウスや通知で戻ります。\n"
         L"{NotchOpen}ノッチがあるとき、隠れたアイランドを開くノッチのラインを選べます（「スタイル」タブ）。ほかのラインは隠れ、そこ"
         L"でのクリックは下のウィンドウに届きます。\n"
         L"{Notches}アイランド スタイルでもノッチを使えます（「スタイル」タブ）。固定スタイルと同じですが画面の帯は確保しません。スタ"
         L"イルごとに選択が保存されます。\n"
         L"{BarSide}バー スタイルを左右の端にも置けます（「外観」タブ）。項目は上から下に並び、画面と接する内側の角を丸くできます。\n"
         L"{BarClear}透明なバー（「外観」タブ）：後ろの上に項目だけ、またはすりガラスでぼかしだけ。\n"
         L"{TextSize}文字サイズに専用のスライダー（「外観」タブ）。アイコンのサイズとは別です。\n"
         L"{AiUsage}カプセルに AI の使用量（「アプリとツール」タブ）：Claude Code（ターミナル）と Codex "
         L"の上限の使用量とリセットまでの時間を、この PC で読み取ります。ログイン不要、通信なし。\n"
         L"{Commands}自分のコマンド（「アプリとツール」タブ）：プログラム、ファイル、フォルダー、リンク、コマンドラインへのショートカ"
         L"ットを、それぞれのアイコン付きでパネルに。どれでもカプセルのボタンにできます。\n"
         L"{FocusedTitle}フォーカス中のアプリの名前の横にウィンドウのタイトルを表示できます（「デスクトップ」タブ）。\n"
         L"{CalendarBigDay}カレンダー "
         L"カード（「時計とカレンダー」タブ）：月の横に大きな日付、アラーム、タイマー、カレンダーのボタン、その日の予定と次の予定。\n"
         L"{WeatherCards}天気の詳細をカードで表示（スマートフォンのように）（「アプリとツール」タブの天気）。\n"
         L"{WeatherUnit}天気を摂氏または華氏で（「アプリとツール」タブの天気）。\n"
         L"{CapsLock}Caps Lock のオン／オフを知らせるバブル（「バナー」タブ）。\n"
         L"{Profiles}自動で切り替わるプロファイル（「全般」タブ、プロファイルの鉛筆）：特定のアプリが前面にあるとき、またはスタイルを"
         L"選んだとき。アプリの後は前のプロファイルに戻るか、そのまま。\n"
         L"モバイル ホットスポットの名前とパスワードを、コントロール センターのホットスポットのページで変更できます。\n"
         L"{Dock}Dock をデスクトップでは表示したままにし、ウィンドウが最大化されているか重なっている間だけ隠せます（アイランドと同じ"
         L"、Windhawk の設定）。\n"
         L"{Dock}Dock を選んだ幅で止められます。残りのアプリはマウス ホイールでスクロールします（箱のように、Windhawk "
         L"の設定）。\n"
         L"{Dock}Dock の拡大、バウンス、「隠すタイミング」を、アプリだけを残した Windows "
         L"タスクバーでも使えます（Windhawk の設定のオプション）。\n"
         L"{BannerPlace}バナーを画面の 9 "
         L"か所のどこかに、小さな画面で選んで表示（「バナー」タブ）。いちばん近い端からスライドします。または今までどおりカプセルから。\n"
         L"{Shelf}ファイルのシェルフ（「デスクトップ」タブ）：ファイルをカプセルのどこかにドロップして置いておき（場所だけでコピーはしま"
         L"せん）、あとでアイコンを開いて別のアプリへドラッグします。\n"
         L"{FolderThumbnails}フォルダー "
         L"パネルとシェルフに画像、動画、ドキュメントのサムネイル（「デスクトップ」タブ）。マウスを 0.5 秒止めると大きなプレビュー。\n"
         L"{TimerLine}アイランドを最小化しているときにタイマー、ポモドーロ、ストップウォッチが動いていると、ラインがリングと時間付き"
         L"の小さなカプセルになります（「非表示」タブ）。「隠すときは時計だけ」がオンなら時間は時計の横に表示。\n"
         L"{LineClick}隠したラインをマウスではなくクリックで開けます（「非表示」タブ）。マウスを乗せると太くなるだけなので、うっかり"
         L"開きません。隠したバーの時計も同様です。\n"
         L"天気で現在地を使えます（天気パネルの「現在地を使う」）。Windows 自身の許可が必要で、約 1 km "
         L"に丸めてから天気サービスに送ります。都市を入力すると都市に戻ります。\n"
         L"アラームに名前と鳴る曜日を設定できます：毎日、平日、週末、任意の曜日、または 1 回のみ（鳴った後に自動でオフ）。\n"
         L"{GlassLight}Liquid Glass に macOS 27 "
         L"のような光と影（「外観」タブ）：つや、面取りした縁、下側の反射光、柔らかい影。パネルのカードもガラスのようになります。Thiago "
         L"Silva さんのアイデア。\n"
         L"アイランドがベトナム語（Tiếng Việt）に対応しました（翻訳: caotu2k5）。\n"
         L"#改善\n"
         L"設定ウィンドウが Windows 11 の見た目になりました：専用のタイトル バー、大きな文字、幅の広いウィンドウでは小さなオプションを 1 行に 2 つ、常に表示されドラッグできるスクロール バー（サイド バーも）。\n"
         L"何かを尋ねたり知らせたりするバブル（オプションに必要なもの、アイコンのヒント、設定のバブル）は、アイランドの不透明度に関係なく不透明です。\n"
         L"Mod のサイト island.caliberda.com.br（「バージョン情報」タブ）：すべてのオプションの説明、ロードマップ、投票。\n"
         L"{Style}設定を整理：新しい「スタイル」「非表示」「コントロール センター」「バナー」タブ。関係するオプションをまとめました。\n"
         L"グレーのオプションには理由が出ます。その行をクリックすると必要なものがすべて表示され、「はい、変更する」で一度にすべてオンにし、「変"
         L"更を元に戻す」で戻せます。\n"
         L"選択肢の名前が切れなくなりました（必要なら複数行に）。場所と角は図で選べます。\n"
         L"「新機能」では、矢印のある機能はマウスを乗せると強調表示され、クリックするとそのオプションを表示します。\n"
         L"{FolderDrag}フォルダー パネルやシェルフからドラッグしたファイル：最初の 1 "
         L"回だけ確認（その後は記憶）、毎回確認、コピー、または移動（「デスクトップ」タブ）。\n"
         L"{MinimizeKey}Win+J（「非表示」タブ）でバー スタイルでもバー全体を隠し、また戻せます。\n"
         L"{ExternalBrightness}ケーブル経由（DDC/CI）の外部モニターの明るさをオフにできます（「コントロール "
         L"センター」タブ）。コントロール センターを開くとちらつくモニターや、リセットされるドック・MST ハブ向けです。\n"
         L"{BannerSize}バナーのサイズ（50% から 200%）、幅、角を選べます（「バナー」タブ）。\n"
         L"{DndSilent}通話、アラーム、リマインダーも応答不可モードで消音にできます（「通知」タブ）。\n"
         L"{DownloadStalled}30 秒止まったブラウザーのダウンロードは一時停止（黄色）として表示されます（「システム」タブ）。"
         L"ブラウザーは一時停止を知らせないためです。\n"
         L"{Reset}エクスポートとインポートの横の「リセット」ボタン（「全般」タブ、ダブルクリック）で、アイランドの設定を既定に戻します。"
         L"メモ、タスク、予定、プロファイルは残ります。\n"
         L"コントロール センターのミキサーに、アプリごとの音量がパーセントで表示されます。\n"
         L"カプセルから直接開いたページに戻る矢印があります。\n"
         L"予定エディターで、そのリマインダーが何をするかが表示されます。\n"
         L"カレンダー カードの日付の上に、その月の名前が表示されます。\n"
         L"#修正\n"
         L"Dock は、タスク バーにピン留めしたショートカットに設定したアイコンを、アプリ本来のアイコンに戻さず保ちます (#6891)。\n"
         L"下に別の拡大率のモニターがあるとき、タスク バーのモードを切り替えても Dock が中央からずれて切れなくなりました。\n"
         L"設定ウィンドウでの Alt や F10、ウィンドウの移動で、アイランドのアニメーションが止まらなくなりました。\n"
         L"アイランドが開けないプログラム（NVIDIA など）のトレイ アイコンは、Mod をオフにしてオンに戻しても画像が残ります。\n"
         L"別のモニターのアイランドの天気パネルが「検索中」のままにならなくなりました。\n"
         L"「変更を元に戻す」はアイランドの再起動後も残り、その後に変更されたオプションには触れません。\n"
         L"明るさ、3 つのノッチでの Caps Lock、シェルフの数、他のモニターの Dock の「ウィンドウが重なったとき」が正しく動作します。\n"
         L"Windhawk を管理者として実行しても、ほかのアプリのトレイ "
         L"アイコン、バッジ、ダウンロードの進行状況がアイランドに再び表示されます。\n"
         L"アニメーションがオフでも、パネルを開くときにカプセルがちらつかず、電源ボタンの確認が表示されます。\n"
         L"ポータブル版 Windhawk でもショートカットとメモの改行が保たれます。\n"
         L"Apple Music（アプリ、Cider、Web ページ）と、曲の長さを伝えないプレーヤーの歌詞に対応。\n"
         L"1 つのアイランド（ノッチや別のモニターのもの）で見た通知のアプリは、すべてのアイランドで既読になります。\n"
         L"タスクバーが自動的に隠れた後、Dock や透明なタスクバーの下に白い帯が出なくなりました。\n"
         L"画像を滑らかに縮小できないグラフィック ドライバーでも、アイランドが空にならなくなりました。\n"
         L"時計が中央のバー スタイルで、項目が時計の上でちらつかなくなりました。\n"
         L"エクスプローラーがアイランドより後に起動しても、ごみ箱の数が更新されます。\n"
         L"コントロール センターで電源モードを変えても、アイランドが止まらなくなりました。\n"
         L"横に移したバーが画面の上端を返すようになりました（再び移すと横も返します）。\n"
         L"HDR モニターでは、明るさのスライダーでデスクトップの明るさ（Windows の SDR コンテンツの明るさ）が変わります。\n"
         L"アプリごとの位置とアイコンは、デスクトップとタスクバーでは最後のアプリのものではなく共通の設定に戻ります。\n"
         L"下に別のモニターがあるとき、隠れた Dock は画面の端の外へ滑って消え、端の上の線に縮まなくなりました。\n"
         L"ほかに何も知らせない入力方式（最新の Microsoft Pinyin など）でも、「中/英」項目が Shift に追従します。\n"
         L"一部の項目ができる前に選んだアプリ独自のアイコンは、オフの項目を表示しなくなりました。\n"
         L"コントロール センターの使用率ページは、カプセルに項目がなくても CPU、メモリ、VRAM "
         L"を表示し、それぞれにタブがあります。\n"
         L"モニターが 2 "
         L"台のとき、どのウィンドウにフォーカスがあっても、自分のモニターに全画面ウィンドウがある間はアイランドが隠れたままです。\n"
         L"同じアプリ内で切り替えたキーボードが言語の項目に表示されます。\n"
         L"マウスが画面下端に沿って Dock まで移動すると、Dock が表示されます。\n"
         L"デスクトップ アプリの通知をクリックすると、通知が示す場所に移動します（登録しているアプリの場合）。\n"
         L"パネルがカプセルの角に合わせられ、スクリーンショット後にカプセルが変わらなくなりました。\n"
         L"Dock でクリックしたアプリは、クリックからウィンドウが表示されるまで跳ねます。",
         L"#Tính năng mới\n"
         L"{Setup}Trợ lý thiết lập (tab Trợ lý thiết lập, dưới Giới thiệu) dẫn bạn qua các lựa chọn chính: kiểu dáng, các mục trên thanh đảo, tự ẩn, thông báo và giao diện, mỗi bước hiện ngay và được hoàn tác bằng Hủy; nó tự mở khi mới cài.\n"
         L"{IconPack}Bộ icon (tab Biểu tượng): một trong 12 bộ (Fluent, Lucide, Material, Phosphor, Emoji và nhiều bộ khác, chỉ tải một lần) hoặc một thư mục ảnh của riêng bạn; từng icon cũng có thể có ảnh hoặc màu riêng.\n"
         L"{HideWhen}Chọn khi nào đảo ẩn thành vạch mảnh (tab Tự ẩn): khi chuột rời đi, chỉ khi có cửa sổ phóng to, hoặc chỉ khi có cửa sổ đè lên; trên màn hình nền nó vẫn hiện.\n"
         L"{HideClock}Chỉ hiện đồng hồ khi ẩn (tab Tự ẩn): thay cho vạch mảnh, một thanh đảo nhỏ có giờ ở lại; nó thành vạch mảnh khi có cửa sổ nằm dưới (kiểu Đảo và Cố định). Thanh ngang ẩn bằng Win+J cũng thu vào chính thanh nhỏ đó; đưa chuột vào sẽ mở lại thanh.\n"
         L"{LineFade}Vạch mảnh có thể mờ dần sau 30 giây, 2 phút hoặc 10 phút không ai đụng tới (tab Tự ẩn) để không bị in hằn trên màn hình OLED; chuột hoặc một thông báo sẽ gọi nó lại.\n"
         L"{NotchOpen}Với tai thỏ, chọn vạch của tai thỏ nào sẽ mở đảo đang ẩn (tab Kiểu dáng); các vạch còn lại được ẩn và bấm vào đó sẽ chuyển xuống cửa sổ bên dưới.\n"
         L"{Notches}Tai thỏ cũng có ở kiểu Đảo (tab Kiểu dáng), như của kiểu Cố định nhưng không chừa dải màn hình; mỗi kiểu giữ lựa chọn riêng.\n"
         L"{BarSide}Kiểu Thanh ngang cũng có thể đặt ở cạnh trái hoặc phải (tab Giao diện), các mục xếp từ trên xuống, với góc trong được bo ở chỗ giáp màn hình.\n"
         L"{BarClear}Thanh trong suốt (tab Giao diện): chỉ các mục của thanh nằm trên nền phía sau, hoặc với kính mờ thì chỉ còn lớp nhòe.\n"
         L"{TextSize}Cỡ chữ có thanh trượt riêng (tab Giao diện), tách khỏi cỡ icon.\n"
         L"{AiUsage}Mức dùng AI trên thanh đảo (tab Ứng dụng & công cụ): đã dùng bao nhiêu giới hạn của Claude Code (trong terminal) và Codex và khi nào được làm mới, đọc ngay trên máy này, không cần đăng nhập và không gửi gì lên mạng.\n"
         L"{Commands}Lệnh của bạn (tab Ứng dụng & công cụ): lối tắt tới chương trình, tệp, thư mục, link hoặc dòng lệnh, có icon riêng, trong một bảng; mục nào cũng có thể thành nút trên thanh đảo.\n"
         L"{FocusedTitle}Ứng dụng đang dùng có thể hiện tiêu đề cửa sổ bên cạnh tên (tab Màn hình nền).\n"
         L"{CalendarBigDay}Thẻ lịch (tab Đồng hồ & lịch): ngày to bên cạnh tháng, các nút báo thức, hẹn giờ và lịch đăng ký, sự kiện trong ngày và những gì sắp tới.\n"
         L"{WeatherCards}Chi tiết thời tiết dạng thẻ, như trên điện thoại (tab Ứng dụng & công cụ, mục Thời tiết).\n"
         L"{WeatherUnit}Độ C hoặc độ F cho thời tiết (tab Ứng dụng & công cụ, mục Thời tiết).\n"
         L"{CapsLock}Khung nhỏ báo khi Caps Lock bật hoặc tắt (tab Thông báo nổi).\n"
         L"{Profiles}Hồ sơ tự bật (tab Chung, bút chì của hồ sơ): khi một số ứng dụng đang dùng hoặc khi chọn một kiểu dáng; sau khi ứng dụng đóng, nó về hồ sơ trước đó hoặc giữ nguyên.\n"
         L"Tên và mật khẩu của điểm phát sóng di động có thể đổi trong trung tâm điều khiển, ở trang của điểm phát sóng.\n"
         L"{Dock}Dock có thể ở yên trên màn hình nền và chỉ ẩn khi có cửa sổ phóng to hoặc đè lên, như đảo (cài đặt của Windhawk).\n"
         L"{Dock}Dock có thể dừng ở độ rộng bạn chọn; các ứng dụng còn lại cuộn bằng con lăn chuột, như một cái hộp (cài đặt của Windhawk).\n"
         L"{Dock}Độ phóng đại, hiệu ứng nảy và \"Khi nào ẩn\" của Dock cũng dùng được trên taskbar Windows chỉ giữ lại ứng dụng (một tùy chọn trong cài đặt của Windhawk).\n"
         L"{BannerPlace}Thông báo nổi ở một trong chín điểm trên màn hình, chọn trên một màn hình nhỏ (tab Thông báo nổi), trượt vào từ mép gần nhất; hoặc bung ra từ thanh đảo như trước.\n"
         L"{Shelf}Khay tệp (tab Màn hình nền): thả tệp vào bất kỳ đâu trên thanh đảo để giữ ở đó (chỉ lưu vị trí, không tạo bản sao), rồi mở icon của nó và kéo sang ứng dụng khác.\n"
         L"{FolderThumbnails}Ảnh thu nhỏ của ảnh, video và tài liệu trong bảng thư mục và trên khay tệp (tab Màn hình nền); để chuột nửa giây trên một mục để xem bản xem trước lớn hơn.\n"
         L"{TimerLine}Khi hẹn giờ, Pomodoro hoặc bấm giờ đang chạy lúc đảo thu gọn, vạch mảnh thành một thanh đảo nhỏ có vòng tròn và thời gian (tab Tự ẩn); với \"Khi ẩn chỉ hiện đồng hồ\", thời gian nằm cạnh đồng hồ.\n"
         L"{LineClick}Vạch mảnh đang ẩn có thể mở bằng cách bấm thay vì đưa chuột vào (tab Tự ẩn): nó chỉ phình ra khi chuột ở dưới nên đảo không mở nhầm; đồng hồ của Thanh ngang đang ẩn cũng vậy.\n"
         L"Thời tiết có thể dùng vị trí của bạn (\"Dùng vị trí của tôi\" trong bảng thời tiết), với quyền của chính Windows; vị trí được làm tròn khoảng một km trước khi gửi tới dịch vụ thời tiết, và nhập tên thành phố sẽ quay lại cách cũ.\n"
         L"Báo thức có thể có tên và các ngày kêu: mỗi ngày, ngày thường, cuối tuần, ngày bất kỳ, hoặc một lần (tự tắt sau khi kêu).\n"
         L"{GlassLight}Ánh sáng và bóng đổ như macOS 27 cho Liquid Glass (tab Giao diện): độ bóng, cạnh vát, ánh sáng dọc mép dưới và bóng đổ mềm, các thẻ trong bảng cũng trông như kính. Ý tưởng của Thiago Silva.\n"
         L"Đảo có tiếng Việt, do caotu2k5 dịch.\n"
         L"#Cải tiến\n"
         L"Cửa sổ cài đặt trông giống Windows 11: thanh tiêu đề riêng, chữ lớn hơn, các tùy chọn nhỏ hai cái một hàng khi cửa sổ rộng và thanh cuộn luôn hiện, kéo được (cả thanh bên).\n"
         L"Các bong bóng hỏi hoặc báo điều gì đó (thứ một tùy chọn cần, gợi ý của icon, bong bóng cài đặt) luôn đục, dù độ mờ của đảo là bao nhiêu.\n"
         L"Trang web của mod, island.caliberda.com.br (tab Giới thiệu): giải thích từng tùy chọn, lộ trình và lượt bình chọn của bạn.\n"
         L"{Style}Sắp xếp lại cài đặt: thêm các tab Kiểu dáng, Tự ẩn, Trung tâm điều khiển và Thông báo nổi, gom các tùy chọn phụ thuộc nhau lại với nhau.\n"
         L"Tùy chọn bị làm mờ sẽ nói lý do; bấm vào dòng đó để xem mọi thứ nó cần, \"Đồng ý, đổi\" bật hết một lần, còn \"Khôi phục phần đã đổi\" sẽ trả lại.\n"
         L"Các lựa chọn không bao giờ bị cắt tên (xuống thêm hàng khi cần), và vị trí, góc được chọn trên hình vẽ.\n"
         L"Trong Có gì mới, tính năng có mũi tên sẽ sáng lên khi chuột đi qua, và bấm vào sẽ hiện tùy chọn của nó.\n"
         L"{FolderDrag}Tệp kéo ra khỏi bảng thư mục hoặc khay tệp: hỏi lần đầu (rồi nhớ), hỏi mỗi lần, sao chép hoặc di chuyển (tab Màn hình nền).\n"
         L"{MinimizeKey}Win+J (tab Tự ẩn) cũng ẩn cả thanh ở kiểu Thanh ngang, và gọi lại.\n"
         L"{ExternalBrightness}Có thể tắt chỉnh độ sáng màn hình ngoài qua cáp (DDC/CI) (tab Trung tâm điều khiển), cho màn hình bị nhấp nháy hoặc dock và hub MST bị reset khi mở trung tâm điều khiển.\n"
         L"{BannerSize}Có thể chọn kích thước (50% đến 200%), độ rộng và góc của thông báo nổi (tab Thông báo nổi).\n"
         L"{DndSilent}Cuộc gọi, báo thức và nhắc việc cũng có thể im lặng khi Không làm phiền (tab Thông báo).\n"
         L"{DownloadStalled}Tải xuống của trình duyệt đứng yên 30 giây sẽ hiện là tạm dừng, màu vàng (tab Hệ thống): trình duyệt không bao giờ báo tải xuống đã tạm dừng.\n"
         L"{Reset}Nút Đặt lại bên cạnh Xuất và Nhập (tab Chung, bấm hai lần) đưa cài đặt của đảo về như lúc mới có; ghi chú, công việc, sự kiện và hồ sơ được giữ lại.\n"
         L"Bộ trộn âm của trung tâm điều khiển hiện âm lượng từng ứng dụng theo phần trăm.\n"
         L"Các trang mở thẳng từ thanh đảo có mũi tên quay lại.\n"
         L"Trình sửa sự kiện cho biết lời nhắc sẽ làm gì.\n"
         L"Thẻ lịch ghi tên tháng mà các ngày đang hiện, phía trên chúng.\n"
         L"#Sửa lỗi\n"
         L"Dock giữ icon đã đặt cho lối tắt ghim trên taskbar thay vì icon gốc của ứng dụng (#6891).\n"
         L"Khi có màn hình tỉ lệ khác bên dưới, Dock không còn bị cắt lệch tâm sau khi đổi chế độ taskbar.\n"
         L"Alt hoặc F10 trong cửa sổ cài đặt, hay kéo cửa sổ đi, không còn làm dừng hiệu ứng của đảo.\n"
         L"Icon khay của chương trình mà đảo không mở được (như của NVIDIA) vẫn giữ hình sau khi tắt rồi bật lại mod.\n"
         L"Bảng thời tiết trên đảo của màn hình khác không còn kẹt ở Đang tìm.\n"
         L"\"Khôi phục phần đã đổi\" vẫn còn sau khi các đảo khởi động lại, và không đụng tới tùy chọn đã được đổi lại sau đó.\n"
         L"Độ sáng, Caps Lock với ba tai thỏ, số đếm của khay tệp và \"Cửa sổ đè lên\" trên Dock của màn hình khác hoạt động đúng.\n"
         L"Khi Windhawk chạy với quyền quản trị, icon khay hệ thống của ứng dụng khác, huy hiệu của chúng và tiến trình tải xuống lại hiện trên đảo.\n"
         L"Khi tắt hiệu ứng, các bảng mở ra mà thanh đảo không nhấp nháy, và hộp xác nhận của nút nguồn vẫn hiện.\n"
         L"Lối tắt và ghi chú giữ nguyên xuống dòng trong Windhawk portable.\n"
         L"Lời bài hát cho Apple Music (ứng dụng, Cider và trang web) và cho trình phát không báo độ dài bài hát.\n"
         L"Ứng dụng có thông báo thấy trên một đảo (tai thỏ hoặc màn hình khác) sẽ thấy trên mọi đảo.\n"
         L"Không còn dải trắng dưới Dock hoặc taskbar trong suốt sau khi taskbar tự ẩn.\n"
         L"Đảo không còn bị trống với trình điều khiển đồ họa không thu nhỏ ảnh theo cách mượt.\n"
         L"Ở kiểu Thanh ngang với đồng hồ ở giữa, các mục không còn nhấp nháy đè lên đồng hồ.\n"
         L"Số đếm của Thùng rác vẫn cập nhật khi Explorer khởi động sau đảo.\n"
         L"Đổi chế độ nguồn trong trung tâm điều khiển không còn làm đảo bị treo.\n"
         L"Thanh dời sang một bên trả lại cạnh trên của màn hình (và cạnh bên, khi nó dời lần nữa).\n"
         L"Trên màn hình HDR, thanh trượt độ sáng đổi độ sáng của màn hình nền (độ sáng nội dung SDR của Windows).\n"
         L"Vị trí và icon riêng cho từng ứng dụng quay về cài đặt chung trên màn hình nền và taskbar, thay vì giữ của ứng dụng cuối cùng.\n"
         L"Khi có màn hình khác bên dưới, Dock đang ẩn trượt khỏi mép màn hình thay vì co lại thành vạch phía trên nó.\n"
         L"Mục 中/英 đi theo Shift với các bộ gõ không báo gì thêm, như Microsoft Pinyin mới nhất.\n"
         L"Icon riêng của một ứng dụng, chọn trước khi một số mục tồn tại, không còn hiện các mục đó khi chúng đang tắt.\n"
         L"Trang mức dùng của trung tâm điều khiển hiện CPU, RAM và VRAM mà không cần các mục trên thanh đảo, mỗi loại một tab.\n"
         L"Với hai màn hình, một đảo vẫn ẩn khi màn hình của nó có cửa sổ toàn màn hình, dù cửa sổ nào đang giữ tiêu điểm.\n"
         L"Bàn phím đổi trong cùng một ứng dụng sẽ hiện trên mục ngôn ngữ.\n"
         L"Dock hiện lên khi chuột trượt dọc cạnh dưới vào nó.\n"
         L"Bấm vào thông báo của ứng dụng máy tính sẽ đưa tới nơi nó chỉ, với các ứng dụng có đăng ký.\n"
         L"Các bảng theo góc của thanh đảo, và thanh đảo không còn đổi sau khi chụp màn hình.\n"
         L"Ứng dụng bấm trong Dock nảy lên từ lúc bấm cho đến khi cửa sổ của nó hiện."}},
    {L"2.1.0",
     {
         L"#New\n"
         L"A mixer in the media panel: each app's volume and mute, the "
         L"playing app's own volume under the player, and a button for "
         L"Windows' mixer.\n"
         L"The media panel takes its colors from the song's cover, and each "
         L"source shows its app's icon.\n"
         L"{Lyrics}Song lyrics: the line being sung on the pill and the "
         L"whole song in a panel (What's playing tab; from lrclib.net, online).\n"
         L"{Timer}Timer, stopwatch, Pomodoro and alarms (Apps and tools "
         L"tab): what's running shows its time on the pill, and a click "
         L"lists them with Pause and Cancel, like Live Activities.\n"
         L"{Visualizer}An audio visualizer (What's playing tab): bars that "
         L"jump with the beat or sway smoothly like the iPhone's, in the "
         L"cover's color or your own.\n"
         L"{NetSpeed}The network speed on the pill: what's sent and received "
         L"each second (Apps and tools tab).\n"
         L"{WorldClocks}World clocks: Windows' additional clocks on the pill "
         L"(Clock and calendar tab).\n"
         L"{Countdown}The time until your next event, alarm, timer or "
         L"Pomodoro part, on the pill (Clock and calendar tab).\n"
         L"{RecycleBin}The Recycle Bin on the pill: how many items it holds; "
         L"a click opens it or empties it (Desktop tab).\n"
         L"{DownloadFill}The pill fills like a loading bar with downloads, "
         L"file copies or the battery charging (System tab).\n"
         L"{StatusNotices}Notices when the charger is plugged in, the "
         L"battery gets low, Bluetooth audio connects or Do not disturb "
         L"changes (Banners tab).\n"
         L"Settings in a window of their own, with a search, tabs and this "
         L"What's new page.\n"
         L"{Language}The island in German, Spanish, Russian and Japanese too "
         L"(General tab).\n"
         L"#Improvements\n"
         L"Smoother animations: the pill keeps its size while a panel is "
         L"open, and panels close without cutting their sides.\n"
         L"{Folders}The folders panel sorts by name, date, size or type, "
         L"either way (Desktop tab).\n"
         L"{Folders}Favorite files: a star on a file keeps it with the "
         L"pinned folders, to open it or copy it or its path.\n"
         L"{Dock}The dock: profile pictures on icons stay sharp, and apps "
         L"with more windows no longer show an extra plate.\n"
         L"#Fixes\n"
         L"Turning the mod off puts the taskbar's auto-hide and icons back "
         L"as they were.\n"
         L"The loading bar no longer gets stuck after an app finishes, or "
         L"right after Windows starts.",
         L"#Novidades\n"
         L"Um mixer no painel de mídia: o volume e o mudo de cada app, o "
         L"volume próprio do app que toca embaixo do player e um botão para "
         L"o mixer do Windows.\n"
         L"O painel de mídia pega as cores da capa da música, e cada fonte "
         L"mostra o ícone do app dela.\n"
         L"{Lyrics}Letras das músicas: a linha cantada na pílula e a música "
         L"inteira num painel (aba O que está tocando; do lrclib.net, online).\n"
         L"{Timer}Timer, cronômetro, Pomodoro e alarmes (aba Apps e "
         L"ferramentas): o que está rodando mostra o tempo na pílula, e um "
         L"clique lista tudo com Pausar e Cancelar, como as Atividades ao Vivo.\n"
         L"{Visualizer}Um visualizador de áudio (aba O que está tocando): "
         L"barras que pulam com a batida ou balançam suaves como no iPhone, "
         L"na cor da capa ou numa sua.\n"
         L"{NetSpeed}A velocidade da rede na pílula: o que é enviado e "
         L"recebido a cada segundo (aba Apps e ferramentas).\n"
         L"{WorldClocks}Relógios mundiais: os relógios adicionais do Windows "
         L"na pílula (aba Relógio e calendário).\n"
         L"{Countdown}O tempo até o próximo evento, alarme, timer ou parte "
         L"do Pomodoro, na pílula (aba Relógio e calendário).\n"
         L"{RecycleBin}A Lixeira na pílula: quantos itens ela tem; um clique "
         L"abre ou esvazia (aba Área de trabalho).\n"
         L"{DownloadFill}A pílula enche como uma barra de carregamento com "
         L"downloads, cópias de arquivos ou a bateria carregando (aba Sistema).\n"
         L"{StatusNotices}Avisos quando o carregador é ligado, a bateria "
         L"fica fraca, um áudio Bluetooth conecta ou o Não incomodar muda "
         L"(aba Avisos).\n"
         L"Configurações numa janela própria, com busca, abas e esta página "
         L"de Novidades.\n"
         L"{Language}A ilha também em alemão, espanhol, russo e japonês (aba "
         L"Geral).\n"
         L"#Melhorias\n"
         L"Animações mais suaves: a pílula mantém o tamanho enquanto um "
         L"painel está aberto, e os painéis fecham sem cortar as laterais.\n"
         L"{Folders}O painel de pastas ordena por nome, data, tamanho ou "
         L"tipo, nos dois sentidos (aba Área de trabalho).\n"
         L"{Folders}Arquivos favoritos: uma estrela num arquivo o guarda "
         L"junto das pastas fixadas, para abrir ou copiar ele ou o caminho "
         L"dele.\n"
         L"{Dock}A dock: fotos de perfil nos ícones ficam nítidas, e apps "
         L"com mais janelas não mostram mais uma placa extra.\n"
         L"#Correções\n"
         L"Desligar o mod devolve o esconder automático e os ícones da barra "
         L"de tarefas como estavam.\n"
         L"A barra de carregamento não trava mais depois que um app termina "
         L"ou logo depois que o Windows inicia.",
         L"#新功能\n"
         L"媒体面板中的混音器：每个应用的音量和静音、播放器下方正在播放应用的单独音量，以及打开 Windows 混音器的按钮。\n"
         L"媒体面板会采用歌曲封面的颜色，每个来源显示其应用的图标。\n"
         L"{Lyrics}歌词：胶囊上显示正在唱的那一行，面板中显示整首歌（“正在播放”选项卡；来自 lrclib.net，需联网）。\n"
         L"{Timer}计时器、秒表、番茄钟和闹钟（“应用和工具”选项卡）：正在运行的会在胶囊上显示时间，单击会列出它们并提供暂停和取消，就像"
         L"实时活动。\n"
         L"{Visualizer}音频可视化（“正在播放”选项卡）：随节拍跳动或像 iPhone 那样平滑摆动的条形，使用封面颜色或你自己的颜色。\n"
         L"{NetSpeed}胶囊上的网速：每秒发送和接收的数据（“应用和工具”选项卡）。\n"
         L"{WorldClocks}世界时钟：Windows 的附加时钟显示在胶囊上（“时钟和日历”选项卡）。\n"
         L"{Countdown}距离下一个事件、闹钟、计时器或番茄钟阶段的时间，显示在胶囊上（“时钟和日历”选项卡）。\n"
         L"{RecycleBin}胶囊上的回收站：显示其中的项目数；单击可打开或清空（“桌面”选项卡）。\n"
         L"{DownloadFill}胶囊会像进度条一样随下载、文件复制或电池充电而填充（“系统”选项卡）。\n"
         L"{StatusNotices}插入充电器、电量不足、蓝牙音频连接或勿扰模式变化时的提示（“横幅”选项卡）。\n"
         L"设置有了独立窗口，带搜索、选项卡和这个“新功能”页面。\n"
         L"{Language}灵动岛新增德语、西班牙语、俄语和日语（“常规”选项卡）。\n"
         L"#改进\n"
         L"动画更流畅：面板打开时胶囊保持大小，面板关闭时不再裁掉两侧。\n"
         L"{Folders}文件夹面板可按名称、日期、大小或类型正序或倒序排列（“桌面”选项卡）。\n"
         L"{Folders}收藏文件：给文件加星标可将其与固定的文件夹放在一起，用于打开或复制文件及其路径。\n"
         L"{Dock}程序坞：图标上的头像保持清晰，有多个窗口的应用不再显示多余的底板。\n"
         L"#修复\n关闭模组后，任务栏的自动隐藏和图标会恢复原样。\n应用完成后或 Windows 刚启动时，进度条不再卡住。",
         L"#Neu\n"
         L"Ein Mixer im Medienbereich: Lautstärke und Stummschaltung jeder "
         L"App, die eigene Lautstärke der spielenden App unter dem Player "
         L"und ein Knopf für den Windows-Mixer.\n"
         L"Der Medienbereich übernimmt die Farben des Covers, und jede "
         L"Quelle zeigt das Symbol ihrer App.\n"
         L"{Lyrics}Liedtexte: die gesungene Zeile in der Kapsel und das "
         L"ganze Lied in einem Bereich (Tab „Wiedergabe“; von lrclib.net, "
         L"online).\n"
         L"{Timer}Timer, Stoppuhr, Pomodoro und Wecker (Tab „Apps und "
         L"Tools“): was läuft, zeigt seine Zeit in der Kapsel, und ein Klick "
         L"listet alles mit Pause und Abbrechen auf, wie Live-Aktivitäten.\n"
         L"{Visualizer}Ein Audio-Visualizer (Tab „Wiedergabe“): Balken, die "
         L"im Takt springen oder sanft schwingen wie beim iPhone, in der "
         L"Farbe des Covers oder einer eigenen.\n"
         L"{NetSpeed}Die Netzwerkgeschwindigkeit in der Kapsel: was pro "
         L"Sekunde gesendet und empfangen wird (Tab „Apps und Tools“).\n"
         L"{WorldClocks}Weltuhren: die zusätzlichen Uhren von Windows in der "
         L"Kapsel (Tab „Uhr und Kalender“).\n"
         L"{Countdown}Die Zeit bis zum nächsten Termin, Wecker, Timer oder "
         L"Pomodoro-Abschnitt, in der Kapsel (Tab „Uhr und Kalender“).\n"
         L"{RecycleBin}Der Papierkorb in der Kapsel: wie viele Elemente er "
         L"enthält; ein Klick öffnet oder leert ihn (Tab „Desktop“).\n"
         L"{DownloadFill}Die Kapsel füllt sich wie ein Ladebalken mit "
         L"Downloads, Dateikopien oder dem Laden des Akkus (Tab „System“).\n"
         L"{StatusNotices}Hinweise, wenn das Ladegerät angesteckt wird, der "
         L"Akku schwach wird, Bluetooth-Audio sich verbindet oder sich "
         L"„Nicht stören“ ändert (Tab „Banner“).\n"
         L"Einstellungen in einem eigenen Fenster, mit Suche, Tabs und "
         L"dieser Seite „Neuigkeiten“.\n"
         L"{Language}Die Insel auch auf Deutsch, Spanisch, Russisch und "
         L"Japanisch (Tab „Allgemein“).\n"
         L"#Verbesserungen\n"
         L"Flüssigere Animationen: die Kapsel behält ihre Größe, solange ein "
         L"Bereich offen ist, und Bereiche schließen sich, ohne ihre Seiten "
         L"abzuschneiden.\n"
         L"{Folders}Der Ordnerbereich sortiert nach Name, Datum, Größe oder "
         L"Typ, in beide Richtungen (Tab „Desktop“).\n"
         L"{Folders}Favorisierte Dateien: ein Stern auf einer Datei behält "
         L"sie bei den angehefteten Ordnern, um sie zu öffnen oder sie oder "
         L"ihren Pfad zu kopieren.\n"
         L"{Dock}Das Dock: Profilbilder auf Symbolen bleiben scharf, und "
         L"Apps mit mehreren Fenstern zeigen keine zusätzliche Platte mehr.\n"
         L"#Korrekturen\n"
         L"Das Ausschalten des Mods stellt das automatische Ausblenden und "
         L"die Symbole der Taskleiste wieder her.\n"
         L"Der Ladebalken hängt nicht mehr, nachdem eine App fertig ist oder "
         L"direkt nach dem Start von Windows.",
         L"#Novedades\n"
         L"Un mezclador en el panel de medios: el volumen y el silencio de "
         L"cada app, el volumen propio de la app que suena bajo el "
         L"reproductor y un botón para el mezclador de Windows.\n"
         L"El panel de medios toma los colores de la portada de la canción, "
         L"y cada fuente muestra el icono de su app.\n"
         L"{Lyrics}Letras de canciones: la línea que se canta en la píldora "
         L"y la canción entera en un panel (pestaña Reproduciendo; de "
         L"lrclib.net, en línea).\n"
         L"{Timer}Temporizador, cronómetro, Pomodoro y alarmas (pestaña Apps "
         L"y herramientas): lo que está en marcha muestra su tiempo en la "
         L"píldora, y un clic lo lista con Pausar y Cancelar, como las "
         L"Actividades en vivo.\n"
         L"{Visualizer}Un visualizador de audio (pestaña Reproduciendo): "
         L"barras que saltan con el ritmo o se mecen suaves como en el "
         L"iPhone, en el color de la portada o en el tuyo.\n"
         L"{NetSpeed}La velocidad de la red en la píldora: lo enviado y "
         L"recibido cada segundo (pestaña Apps y herramientas).\n"
         L"{WorldClocks}Relojes mundiales: los relojes adicionales de "
         L"Windows en la píldora (pestaña Reloj y calendario).\n"
         L"{Countdown}El tiempo hasta tu próximo evento, alarma, "
         L"temporizador o parte del Pomodoro, en la píldora (pestaña Reloj y "
         L"calendario).\n"
         L"{RecycleBin}La Papelera en la píldora: cuántos elementos tiene; "
         L"un clic la abre o la vacía (pestaña Escritorio).\n"
         L"{DownloadFill}La píldora se llena como una barra de carga con "
         L"descargas, copias de archivos o la batería cargando (pestaña "
         L"Sistema).\n"
         L"{StatusNotices}Avisos cuando se conecta el cargador, la batería "
         L"baja, se conecta un audio Bluetooth o cambia No molestar (pestaña "
         L"Banners).\n"
         L"La configuración en una ventana propia, con búsqueda, pestañas y "
         L"esta página de Novedades.\n"
         L"{Language}La isla también en alemán, español, ruso y japonés "
         L"(pestaña General).\n"
         L"#Mejoras\n"
         L"Animaciones más suaves: la píldora mantiene su tamaño mientras un "
         L"panel está abierto, y los paneles se cierran sin cortar sus lados.\n"
         L"{Folders}El panel de carpetas ordena por nombre, fecha, tamaño o "
         L"tipo, en ambos sentidos (pestaña Escritorio).\n"
         L"{Folders}Archivos favoritos: una estrella en un archivo lo guarda "
         L"con las carpetas fijadas, para abrirlo o copiarlo a él o su ruta.\n"
         L"{Dock}El dock: las fotos de perfil en los iconos se ven nítidas, "
         L"y las apps con más ventanas ya no muestran una placa extra.\n"
         L"#Correcciones\n"
         L"Desactivar el mod devuelve la ocultación automática y los iconos "
         L"de la barra de tareas como estaban.\n"
         L"La barra de carga ya no se queda atascada cuando una app termina "
         L"o justo después de que Windows arranca.",
         L"#Новое\n"
         L"Микшер на панели мультимедиа: громкость и отключение звука "
         L"каждого приложения, собственная громкость играющего приложения "
         L"под плеером и кнопка микшера Windows.\n"
         L"Панель мультимедиа берёт цвета обложки песни, а каждый источник "
         L"показывает значок своего приложения.\n"
         L"{Lyrics}Тексты песен: строка, которую поют, в капсуле и вся песня "
         L"на панели (вкладка «Сейчас играет»; с lrclib.net, через интернет).\n"
         L"{Timer}Таймер, секундомер, Pomodoro и будильники (вкладка "
         L"«Приложения и инструменты»): то, что идёт, показывает время в "
         L"капсуле, а щелчок выводит список с паузой и отменой, как «Живые "
         L"активности».\n"
         L"{Visualizer}Аудиовизуализатор (вкладка «Сейчас играет»): полоски, "
         L"прыгающие в такт или плавно качающиеся, как на iPhone, в цвете "
         L"обложки или своём.\n"
         L"{NetSpeed}Скорость сети в капсуле: что отправляется и принимается "
         L"каждую секунду (вкладка «Приложения и инструменты»).\n"
         L"{WorldClocks}Мировые часы: дополнительные часы Windows в капсуле "
         L"(вкладка «Часы и календарь»).\n"
         L"{Countdown}Время до следующего события, будильника, таймера или "
         L"этапа Pomodoro, в капсуле (вкладка «Часы и календарь»).\n"
         L"{RecycleBin}Корзина в капсуле: сколько в ней элементов; щелчок "
         L"открывает или очищает её (вкладка «Рабочий стол»).\n"
         L"{DownloadFill}Капсула заполняется, как индикатор загрузки, при "
         L"загрузках, копировании файлов или зарядке батареи (вкладка "
         L"«Система»).\n"
         L"{StatusNotices}Уведомления, когда подключается зарядка, батарея "
         L"садится, подключается Bluetooth-аудио или меняется «Не "
         L"беспокоить» (вкладка «Баннеры»).\n"
         L"Настройки в отдельном окне, с поиском, вкладками и этой страницей "
         L"«Что нового».\n"
         L"{Language}Остров также на немецком, испанском, русском и японском "
         L"(вкладка «Общие»).\n"
         L"#Улучшения\n"
         L"Более плавные анимации: капсула сохраняет размер, пока открыта "
         L"панель, а панели закрываются, не обрезая края.\n"
         L"{Folders}Панель папок сортирует по имени, дате, размеру или типу, "
         L"в обе стороны (вкладка «Рабочий стол»).\n"
         L"{Folders}Избранные файлы: звёздочка на файле держит его рядом с "
         L"закреплёнными папками, чтобы открыть его или скопировать его "
         L"самого или путь.\n"
         L"{Dock}Док: аватары на значках остаются чёткими, а у приложений с "
         L"несколькими окнами больше нет лишней подложки.\n"
         L"#Исправления\n"
         L"Выключение мода возвращает автоскрытие и значки панели задач как "
         L"были.\n"
         L"Индикатор загрузки больше не зависает после завершения приложения "
         L"или сразу после запуска Windows.",
         L"#新機能\n"
         L"メディア パネルにミキサー：アプリごとの音量とミュート、プレーヤーの下に再生中のアプリ自体の音量、Windows "
         L"のミキサーを開くボタン。\n"
         L"メディア パネルが曲のジャケットの色になり、ソースごとにアプリのアイコンを表示します。\n"
         L"{Lyrics}歌詞：歌っている行をカプセルに、曲全体をパネルに表示（「再生中」タブ。lrclib.net から、オンライン）。\n"
         L"{Timer}タイマー、ストップウォッチ、ポモドーロ、アラーム（「アプリとツール」タブ）：動いているものの時間をカプセルに表示し、ク"
         L"リックで一時停止とキャンセル付きの一覧を表示します（ライブ アクティビティのように）。\n"
         L"{Visualizer}オーディオ ビジュアライザー（「再生中」タブ）：ビートに合わせて跳ねる、または iPhone "
         L"のように滑らかに揺れるバー。ジャケットの色か好きな色で。\n"
         L"{NetSpeed}カプセルにネットワーク速度：1 秒ごとの送信量と受信量（「アプリとツール」タブ）。\n"
         L"{WorldClocks}世界時計：Windows の追加の時計をカプセルに表示（「時計とカレンダー」タブ）。\n"
         L"{Countdown}次の予定、アラーム、タイマー、ポモドーロの区切りまでの時間をカプセルに表示（「時計とカレンダー」タブ）。\n"
         L"{RecycleBin}カプセルにごみ箱：中の項目数。クリックで開くか空にします（「デスクトップ」タブ）。\n"
         L"{DownloadFill}ダウンロード、ファイルのコピー、バッテリーの充電で、カプセルが読み込みバーのように満ちます（「システム」"
         L"タブ）。\n"
         L"{StatusNotices}充電器の接続、バッテリー残量の低下、Bluetooth "
         L"オーディオの接続、応答不可の切り替えのお知らせ（「バナー」タブ）。\n"
         L"設定が専用のウィンドウに。検索、タブ、この「新機能」ページ付き。\n"
         L"{Language}アイランドがドイツ語、スペイン語、ロシア語、日本語にも対応（「全般」タブ）。\n"
         L"#改善\n"
         L"アニメーションがより滑らかに：パネルが開いている間はカプセルのサイズが変わらず、パネルは端を切らずに閉じます。\n"
         L"{Folders}フォルダー パネルを名前、日付、サイズ、種類で昇順・降順に並べ替え（「デスクトップ」タブ）。\n"
         L"{Folders}お気に入りのファイル：ファイルに星を付けるとピン留めしたフォルダーと一緒に残り、開いたりファイルやパスをコピーした"
         L"りできます。\n"
         L"{Dock}Dock：アイコン上のプロフィール画像がくっきりし、複数のウィンドウを持つアプリに余分な台座が出なくなりました。\n"
         L"#修正\n"
         L"Mod をオフにすると、タスクバーの自動非表示とアイコンが元に戻ります。\n"
         L"アプリの終了後や Windows の起動直後に、読み込みバーが止まらなくなりました。",
         L"#Tính năng mới\n"
         L"Bộ trộn âm trong bảng phát nhạc: âm lượng và tắt tiếng của từng ứng dụng, âm lượng riêng của ứng dụng đang phát dưới trình phát, và một nút mở bộ trộn của Windows.\n"
         L"Bảng phát nhạc lấy màu từ ảnh bìa bài hát, và mỗi nguồn phát hiện icon ứng dụng của nó.\n"
         L"{Lyrics}Lời bài hát: câu đang hát trên thanh đảo và cả bài trong một bảng (tab Đang phát; lấy từ lrclib.net, cần internet).\n"
         L"{Timer}Hẹn giờ, bấm giờ, Pomodoro và báo thức (tab Ứng dụng & công cụ): mục đang chạy hiện thời gian trên thanh đảo, bấm vào sẽ liệt kê kèm Tạm dừng và Hủy, như Live Activities.\n"
         L"{Visualizer}Sóng nhạc (tab Đang phát): các cột nhảy theo nhịp hoặc đung đưa mượt như của iPhone, theo màu ảnh bìa hoặc màu của bạn.\n"
         L"{NetSpeed}Tốc độ mạng trên thanh đảo: dữ liệu gửi và nhận mỗi giây (tab Ứng dụng & công cụ).\n"
         L"{WorldClocks}Đồng hồ thế giới: các đồng hồ phụ của Windows trên thanh đảo (tab Đồng hồ & lịch).\n"
         L"{Countdown}Thời gian còn lại tới sự kiện, báo thức, hẹn giờ hoặc phiên Pomodoro tiếp theo, trên thanh đảo (tab Đồng hồ & lịch).\n"
         L"{RecycleBin}Thùng rác trên thanh đảo: số mục bên trong; bấm để mở hoặc dọn sạch (tab Màn hình nền).\n"
         L"{DownloadFill}Thanh đảo lấp đầy như thanh tiến trình khi tải xuống, chép tệp hoặc sạc pin (tab Hệ thống).\n"
         L"{StatusNotices}Thông báo khi cắm sạc, pin yếu, kết nối âm thanh Bluetooth hoặc Không làm phiền thay đổi (tab Thông báo nổi).\n"
         L"Cài đặt trong cửa sổ riêng, có tìm kiếm, tab và trang Có gì mới này.\n"
         L"{Language}Đảo cũng có tiếng Đức, Tây Ban Nha, Nga và Nhật (tab Chung).\n"
         L"#Cải tiến\n"
         L"Chuyển động mượt hơn: thanh đảo giữ nguyên kích thước khi một bảng đang mở, và các bảng đóng lại mà không bị cắt hai bên.\n"
         L"{Folders}Bảng thư mục sắp xếp theo tên, ngày, dung lượng hoặc định dạng, theo chiều nào cũng được (tab Màn hình nền).\n"
         L"{Folders}Tệp yêu thích: gắn sao cho tệp để giữ chung với các thư mục đã ghim, để mở hoặc sao chép nó hay đường dẫn của nó.\n"
         L"{Dock}Dock: ảnh đại diện trên icon luôn sắc nét, và ứng dụng có nhiều cửa sổ không còn hiện thêm một lớp nền dư.\n"
         L"#Sửa lỗi\n"
         L"Tắt mod sẽ trả lại đúng trạng thái tự ẩn và icon của taskbar như trước.\n"
         L"Thanh tiến trình không còn bị kẹt sau khi ứng dụng hoàn tất, hoặc ngay sau khi Windows khởi động."}},
    {L"2.0.6",
     {
         L"#New\n"
         L"{Dock}The dock redone like macOS's: the icons magnify in a wave "
         L"on a rounded shelf, and stay sharp while magnified (Windhawk's "
         L"settings).\n"
         L"{Glass}Glass: the island and its panels see-through and blurred, "
         L"with a translucent option (Look tab).\n"
         L"{Language}The island in English, Portuguese or Chinese (General "
         L"tab).\n"
         L"{PillShape}The pill's height, corners, the room at its ends and "
         L"the edge around it can be set (Look tab).\n"
         L"{BarCorners}The Bar style can put its items at the screen's "
         L"corners, with the clock in the middle (Look tab).\n"
         L"#Improvements\n"
         L"A green battery while it charges, and the percentages closer to "
         L"their icons.\n"
         L"The calendar can hide your past events (its gear).\n"
         L"{Updates}Updates are checked as often as you choose (Apps and "
         L"tools tab).\n"
         L"#Fixes\n"
         L"The island starts even when Windows isn't ready at sign-in.\n"
         L"Updating the mod no longer leaves the island off.\n"
         L"Fixes for Wi-Fi, Bluetooth, the keyboard language bubble and "
         L"Discord notifications.",
         L"#Novidades\n"
         L"{Dock}A dock refeita como a do macOS: os ícones ampliam numa onda "
         L"sobre uma base arredondada e ficam nítidos ampliados "
         L"(configurações do Windhawk).\n"
         L"{Glass}Vidro: a ilha e os painéis transparentes e desfocados, com "
         L"uma opção translúcida (aba Aparência).\n"
         L"{Language}A ilha em inglês, português ou chinês (aba Geral).\n"
         L"{PillShape}A altura, os cantos, o espaço nas pontas e a borda em "
         L"volta da pílula podem ser ajustados (aba Aparência).\n"
         L"{BarCorners}O estilo Barra pode pôr os itens nos cantos da tela, "
         L"com o relógio no meio (aba Aparência).\n"
         L"#Melhorias\n"
         L"Uma bateria verde enquanto carrega, e as porcentagens mais perto "
         L"dos ícones.\n"
         L"O calendário pode esconder os seus eventos passados (na "
         L"engrenagem dele).\n"
         L"{Updates}As atualizações são verificadas com a frequência que "
         L"você escolher (aba Apps e ferramentas).\n"
         L"#Correções\n"
         L"A ilha inicia mesmo quando o Windows não está pronto ao entrar na "
         L"conta.\n"
         L"Atualizar o mod não deixa mais a ilha desligada.\n"
         L"Correções no Wi-Fi, no Bluetooth, no balão do idioma do teclado e "
         L"nas notificações do Discord.",
         L"#新功能\n"
         L"{Dock}程序坞像 macOS 一样重做：图标在圆角底座上波浪式放大，放大时也保持清晰（Windhawk 设置）。\n"
         L"{Glass}玻璃：灵动岛和面板透明模糊，并有半透明选项（“外观”选项卡）。\n"
         L"{Language}灵动岛支持英语、葡萄牙语和中文（“常规”选项卡）。\n"
         L"{PillShape}可以调整胶囊的高度、圆角、两端空间和外框（“外观”选项卡）。\n"
         L"{BarCorners}条形样式可以把项目放到屏幕两角，时钟在中间（“外观”选项卡）。\n"
         L"#改进\n"
         L"充电时电池显示为绿色，百分比更靠近图标。\n"
         L"日历可以隐藏你过去的事件（在它的齿轮中）。\n"
         L"{Updates}按你选择的频率检查更新（“应用和工具”选项卡）。\n"
         L"#修复\n"
         L"即使登录时 Windows 尚未就绪，灵动岛也能启动。\n"
         L"更新模组后灵动岛不再保持关闭。\n修复了 Wi-Fi、蓝牙、键盘语言气泡和 Discord 通知的问题。",
         L"#Neu\n"
         L"{Dock}Das Dock neu gemacht wie bei macOS: die Symbole vergrößern "
         L"sich in einer Welle auf einem abgerundeten Regal und bleiben "
         L"vergrößert scharf (Windhawk-Einstellungen).\n"
         L"{Glass}Glas: die Insel und ihre Bereiche durchsichtig und "
         L"unscharf, mit einer durchscheinenden Option (Tab „Aussehen“).\n"
         L"{Language}Die Insel auf Englisch, Portugiesisch oder Chinesisch "
         L"(Tab „Allgemein“).\n"
         L"{PillShape}Höhe, Ecken, Platz an den Enden und der Rand der "
         L"Kapsel sind einstellbar (Tab „Aussehen“).\n"
         L"{BarCorners}Der Leisten-Stil kann seine Elemente in die "
         L"Bildschirmecken setzen, mit der Uhr in der Mitte (Tab „Aussehen“).\n"
         L"#Verbesserungen\n"
         L"Ein grüner Akku beim Laden, und die Prozentangaben näher an ihren "
         L"Symbolen.\n"
         L"Der Kalender kann deine vergangenen Termine ausblenden (sein "
         L"Zahnrad).\n"
         L"{Updates}Updates werden so oft geprüft, wie du willst (Tab „Apps "
         L"und Tools“).\n"
         L"#Korrekturen\n"
         L"Die Insel startet, auch wenn Windows bei der Anmeldung noch nicht "
         L"bereit ist.\n"
         L"Ein Update des Mods lässt die Insel nicht mehr ausgeschaltet.\n"
         L"Korrekturen für WLAN, Bluetooth, die Blase der Tastatursprache "
         L"und Discord-Benachrichtigungen.",
         L"#Novedades\n"
         L"{Dock}El dock rehecho como el de macOS: los iconos se amplían en "
         L"ola sobre un estante redondeado y se ven nítidos ampliados "
         L"(configuración de Windhawk).\n"
         L"{Glass}Vidrio: la isla y sus paneles transparentes y "
         L"desenfocados, con una opción translúcida (pestaña Aspecto).\n"
         L"{Language}La isla en inglés, portugués o chino (pestaña General).\n"
         L"{PillShape}Se pueden ajustar la altura, las esquinas, el espacio "
         L"en los extremos y el borde de la píldora (pestaña Aspecto).\n"
         L"{BarCorners}El estilo Barra puede poner sus elementos en las "
         L"esquinas de la pantalla, con el reloj en el centro (pestaña "
         L"Aspecto).\n"
         L"#Mejoras\n"
         L"Una batería verde mientras carga, y los porcentajes más cerca de "
         L"sus iconos.\n"
         L"El calendario puede ocultar tus eventos pasados (en su engranaje).\n"
         L"{Updates}Las actualizaciones se comprueban con la frecuencia que "
         L"elijas (pestaña Apps y herramientas).\n"
         L"#Correcciones\n"
         L"La isla arranca aunque Windows no esté listo al iniciar sesión.\n"
         L"Actualizar el mod ya no deja la isla apagada.\n"
         L"Correcciones en Wi-Fi, Bluetooth, la burbuja del idioma del "
         L"teclado y las notificaciones de Discord.",
         L"#Новое\n"
         L"{Dock}Док переделан как в macOS: значки увеличиваются волной на "
         L"скруглённой подложке и остаются чёткими при увеличении (настройки "
         L"Windhawk).\n"
         L"{Glass}Стекло: остров и его панели прозрачные и размытые, с "
         L"полупрозрачным вариантом (вкладка «Вид»).\n"
         L"{Language}Остров на английском, португальском или китайском "
         L"(вкладка «Общие»).\n"
         L"{PillShape}Можно настроить высоту, углы, место по краям и рамку "
         L"капсулы (вкладка «Вид»).\n"
         L"{BarCorners}Стиль «Панель» может ставить элементы в углы экрана, "
         L"с часами посередине (вкладка «Вид»).\n"
         L"#Улучшения\n"
         L"Зелёная батарея при зарядке, а проценты ближе к значкам.\n"
         L"Календарь может скрывать ваши прошедшие события (в его шестерёнке).\n"
         L"{Updates}Обновления проверяются так часто, как вы выберете "
         L"(вкладка «Приложения и инструменты»).\n"
         L"#Исправления\n"
         L"Остров запускается, даже если Windows ещё не готова при входе.\n"
         L"Обновление мода больше не оставляет остров выключенным.\n"
         L"Исправления для Wi-Fi, Bluetooth, пузырька языка клавиатуры и "
         L"уведомлений Discord.",
         L"#新機能\n"
         L"{Dock}Dock を macOS 風に作り直しました：丸い台座の上でアイコンが波のように拡大し、拡大中もくっきり（Windhaw"
         L"k の設定）。\n"
         L"{Glass}ガラス：アイランドとパネルを透明でぼかした表示に。半透明のオプション付き（「外観」タブ）。\n"
         L"{Language}アイランドを英語、ポルトガル語、中国語で（「全般」タブ）。\n"
         L"{PillShape}カプセルの高さ、角、両端の余白、縁を調整できます（「外観」タブ）。\n"
         L"{BarCorners}バー スタイルで項目を画面の角に置き、時計を中央にできます（「外観」タブ）。\n"
         L"#改善\n"
         L"充電中はバッテリーが緑色になり、パーセントがアイコンの近くに表示されます。\n"
         L"カレンダーで過去の予定を隠せます（歯車から）。\n"
         L"{Updates}更新の確認間隔を選べます（「アプリとツール」タブ）。\n"
         L"#修正\n"
         L"サインイン時に Windows の準備ができていなくても、アイランドが起動します。\n"
         L"Mod を更新してもアイランドがオフのままになりません。\n"
         L"Wi-Fi、Bluetooth、キーボード言語のバブル、Discord の通知を修正しました。",
         L"#Tính năng mới\n"
         L"{Dock}Dock được làm lại như của macOS: icon phóng đại theo làn sóng trên một nền bo tròn và vẫn sắc nét khi phóng to (cài đặt của Windhawk).\n"
         L"{Glass}Kính: đảo và các bảng trong suốt và nhòe nền, có thêm tùy chọn kính mờ (tab Giao diện).\n"
         L"{Language}Đảo bằng tiếng Anh, tiếng Bồ Đào Nha hoặc tiếng Trung (tab Chung).\n"
         L"{PillShape}Có thể chỉnh độ dày, góc, khoảng đệm hai đầu và viền quanh thanh đảo (tab Giao diện).\n"
         L"{BarCorners}Kiểu Thanh ngang có thể đẩy các mục sang hai góc màn hình, đồng hồ ở giữa (tab Giao diện).\n"
         L"#Cải tiến\n"
         L"Pin màu xanh lá khi đang sạc, và số phần trăm sát icon hơn.\n"
         L"Lịch có thể ẩn sự kiện đã qua của bạn (bánh răng của nó).\n"
         L"{Updates}Bản cập nhật được kiểm tra theo chu kỳ bạn chọn (tab Ứng dụng & công cụ).\n"
         L"#Sửa lỗi\n"
         L"Đảo khởi động được ngay cả khi Windows chưa sẵn sàng lúc đăng nhập.\n"
         L"Cập nhật mod không còn làm đảo bị tắt.\n"
         L"Sửa lỗi Wi-Fi, Bluetooth, khung báo ngôn ngữ bàn phím và thông báo của Discord."}},
    {L"2.0.1",
     {
         L"#Improvements\n"
         L"Panels always open under the item you click.\n"
         L"{IconSize}The icons' size can be set from 70% to 150% (Look tab).\n"
         L"Notches grow with their items.\n"
         L"Each monitor's profile keeps its own options (Monitors tab).\n"
         L"#Fixes\n"
         L"Widgets' news show with their icon, and a click opens Widgets.\n"
         L"Windows' own notifications show their icon.\n"
         L"A panel opened and closed quickly no longer leaves its outline "
         L"behind.",
         L"#Melhorias\n"
         L"Os painéis sempre abrem embaixo do item que você clica.\n"
         L"{IconSize}O tamanho dos ícones pode ir de 70% a 150% (aba "
         L"Aparência).\n"
         L"Os entalhes crescem com os itens.\n"
         L"O perfil de cada monitor guarda as próprias opções (aba Monitores).\n"
         L"#Correções\n"
         L"As notícias dos Widgets aparecem com o ícone, e um clique abre os "
         L"Widgets.\n"
         L"As notificações do próprio Windows mostram o ícone.\n"
         L"Um painel aberto e fechado rápido não deixa mais o contorno na "
         L"tela.",
         L"#改进\n"
         L"面板始终在你单击的项目下方打开。\n"
         L"{IconSize}图标大小可在 70% 到 150% 之间设置（“外观”选项卡）。\n"
         L"刘海会随项目变大。\n"
         L"每台显示器的配置文件保留自己的选项（“显示器”选项卡）。\n"
         L"#修复\n小组件的新闻会显示图标，单击会打开小组件。\nWindows 自身的通知会显示图标。\n快速打开又关闭的面板不再留下轮廓。",
         L"#Verbesserungen\n"
         L"Bereiche öffnen sich immer unter dem Element, das du anklickst.\n"
         L"{IconSize}Die Symbolgröße ist von 70 % bis 150 % einstellbar (Tab "
         L"„Aussehen“).\n"
         L"Notches wachsen mit ihren Elementen.\n"
         L"Das Profil jedes Monitors behält seine eigenen Optionen (Tab "
         L"„Monitore“).\n"
         L"#Korrekturen\n"
         L"Die Nachrichten der Widgets erscheinen mit ihrem Symbol, und ein "
         L"Klick öffnet die Widgets.\n"
         L"Die eigenen Benachrichtigungen von Windows zeigen ihr Symbol.\n"
         L"Ein schnell geöffneter und geschlossener Bereich hinterlässt "
         L"keinen Umriss mehr.",
         L"#Mejoras\n"
         L"Los paneles siempre se abren bajo el elemento en el que haces clic.\n"
         L"{IconSize}El tamaño de los iconos se puede ajustar del 70 % al "
         L"150 % (pestaña Aspecto).\n"
         L"Las muescas crecen con sus elementos.\n"
         L"El perfil de cada monitor guarda sus propias opciones (pestaña "
         L"Monitores).\n"
         L"#Correcciones\n"
         L"Las noticias de los Widgets aparecen con su icono, y un clic abre "
         L"los Widgets.\n"
         L"Las notificaciones propias de Windows muestran su icono.\n"
         L"Un panel abierto y cerrado rápido ya no deja su contorno en "
         L"pantalla.",
         L"#Улучшения\n"
         L"Панели всегда открываются под элементом, по которому вы щёлкнули.\n"
         L"{IconSize}Размер значков можно задать от 70 до 150 % (вкладка "
         L"«Вид»).\n"
         L"Вырезы растут вместе с элементами.\n"
         L"Профиль каждого монитора хранит свои параметры (вкладка «Мониторы»).\n"
         L"#Исправления\n"
         L"Новости виджетов показываются со значком, а щелчок открывает "
         L"виджеты.\n"
         L"Собственные уведомления Windows показывают свой значок.\n"
         L"Быстро открытая и закрытая панель больше не оставляет контур.",
         L"#改善\n"
         L"パネルは常にクリックした項目の下に開きます。\n"
         L"{IconSize}アイコンのサイズを 70% から 150% まで設定できます（「外観」タブ）。\n"
         L"ノッチが項目に合わせて広がります。\n"
         L"各モニターのプロファイルが独自のオプションを保持します（「モニター」タブ）。\n"
         L"#修正\n"
         L"ウィジェットのニュースにアイコンが表示され、クリックでウィジェットが開きます。\n"
         L"Windows 自身の通知にアイコンが表示されます。\nすばやく開いて閉じたパネルの輪郭が残らなくなりました。",
         L"#Cải tiến\n"
         L"Các bảng luôn mở ra ngay dưới mục bạn bấm.\n"
         L"{IconSize}Có thể chỉnh cỡ icon từ 70% đến 150% (tab Giao diện).\n"
         L"Tai thỏ tự mở rộng theo số mục bên trong.\n"
         L"Hồ sơ của mỗi màn hình giữ riêng các tùy chọn của nó (tab Màn hình).\n"
         L"#Sửa lỗi\n"
         L"Tin tức của Widgets hiện kèm icon, và bấm vào sẽ mở Widgets.\n"
         L"Thông báo của chính Windows hiện icon.\n"
         L"Mở rồi đóng nhanh một bảng không còn để lại đường viền."}},
    {L"2.0.0",
     {
         L"#New\n"
         L"{Style}The island: a pill at the top of the screen, notches like "
         L"a MacBook's, the Fixed style or a full bar (Style tab).\n"
         L"{OwnNotifications}Notifications drop out of the pill as banners, "
         L"with replies, and the bell keeps the list (Notifications tab).\n"
         L"A control center (its icon on the pill): Wi-Fi, Bluetooth, each "
         L"app's sound, brightness, Do not disturb and night light.\n"
         L"What's playing, a calendar with your agendas, weather, notes, "
         L"tasks, folders, wallpapers and more, each one an option.\n"
         L"{Dock}A dock like macOS's, or the Windows taskbar made "
         L"transparent (Windhawk's settings).\n"
         L"{Profiles}Profiles, backup, your own colors and an island on each "
         L"monitor (General tab).",
         L"#Novidades\n"
         L"{Style}A ilha: uma pílula no topo da tela, entalhes como os do "
         L"MacBook, o estilo Fixa ou uma barra inteira (aba Estilo).\n"
         L"{OwnNotifications}As notificações saem da pílula como avisos, com "
         L"respostas, e o sino guarda a lista (aba Notificações).\n"
         L"Uma central de controle (o ícone dela na pílula): Wi-Fi, "
         L"Bluetooth, o som de cada app, brilho, Não incomodar e luz noturna.\n"
         L"O que está tocando, um calendário com as suas agendas, clima, "
         L"anotações, tarefas, pastas, papéis de parede e mais, cada um uma "
         L"opção.\n"
         L"{Dock}Uma dock como a do macOS, ou a barra de tarefas do Windows "
         L"transparente (configurações do Windhawk).\n"
         L"{Profiles}Perfis, backup, as suas cores e uma ilha em cada "
         L"monitor (aba Geral).",
         L"#新功能\n"
         L"{Style}灵动岛：屏幕顶部的胶囊、像 MacBook 一样的刘海、固定样式或完整的栏（“样式”选项卡）。\n"
         L"{OwnNotifications}通知以横幅形式从胶囊中弹出，可以回复，铃铛保存通知列表（“通知”选项卡）。\n"
         L"控制中心（胶囊上的图标）：Wi-Fi、蓝牙、每个应用的声音、亮度、勿扰模式和夜间模式。\n"
         L"正在播放、带日程的日历、天气、笔记、任务、文件夹、壁纸等，每一项都可选。\n"
         L"{Dock}像 macOS 一样的程序坞，或透明的 Windows 任务栏（Windhawk 设置）。\n"
         L"{Profiles}配置文件、备份、自定义颜色，以及每台显示器上的灵动岛（“常规”选项卡）。",
         L"#Neu\n"
         L"{Style}Die Insel: eine Kapsel oben am Bildschirm, Notches wie "
         L"beim MacBook, der Stil „Fest“ oder eine ganze Leiste (Tab „Stil“).\n"
         L"{OwnNotifications}Benachrichtigungen fallen als Banner aus der "
         L"Kapsel, mit Antworten, und die Glocke behält die Liste (Tab "
         L"„Benachrichtigungen“).\n"
         L"Ein Kontrollzentrum (sein Symbol in der Kapsel): WLAN, Bluetooth, "
         L"der Ton jeder App, Helligkeit, „Nicht stören“ und Nachtmodus.\n"
         L"Wiedergabe, ein Kalender mit deinen Kalendern, Wetter, Notizen, "
         L"Aufgaben, Ordner, Hintergrundbilder und mehr, jedes eine Option.\n"
         L"{Dock}Ein Dock wie bei macOS oder die Windows-Taskleiste "
         L"durchsichtig (Windhawk-Einstellungen).\n"
         L"{Profiles}Profile, Sicherung, eigene Farben und eine Insel auf "
         L"jedem Monitor (Tab „Allgemein“).",
         L"#Novedades\n"
         L"{Style}La isla: una píldora en la parte superior de la pantalla, "
         L"muescas como las del MacBook, el estilo Fija o una barra completa "
         L"(pestaña Estilo).\n"
         L"{OwnNotifications}Las notificaciones salen de la píldora como "
         L"banners, con respuestas, y la campana guarda la lista (pestaña "
         L"Notificaciones).\n"
         L"Un centro de control (su icono en la píldora): Wi-Fi, Bluetooth, "
         L"el sonido de cada app, brillo, No molestar y luz nocturna.\n"
         L"Lo que suena, un calendario con tus agendas, tiempo, notas, "
         L"tareas, carpetas, fondos de pantalla y más, cada uno una opción.\n"
         L"{Dock}Un dock como el de macOS, o la barra de tareas de Windows "
         L"transparente (configuración de Windhawk).\n"
         L"{Profiles}Perfiles, copia de seguridad, tus colores y una isla en "
         L"cada monitor (pestaña General).",
         L"#Новое\n"
         L"{Style}Остров: капсула вверху экрана, вырезы как у MacBook, "
         L"закреплённый стиль или целая панель (вкладка «Стиль»).\n"
         L"{OwnNotifications}Уведомления выпадают из капсулы баннерами, с "
         L"ответами, а колокольчик хранит список (вкладка «Уведомления»).\n"
         L"Центр управления (его значок в капсуле): Wi-Fi, Bluetooth, звук "
         L"каждого приложения, яркость, «Не беспокоить» и ночной свет.\n"
         L"Что играет, календарь с вашими календарями, погода, заметки, "
         L"задачи, папки, обои и другое, каждое по выбору.\n"
         L"{Dock}Док как в macOS или прозрачная панель задач Windows "
         L"(настройки Windhawk).\n"
         L"{Profiles}Профили, резервная копия, свои цвета и остров на каждом "
         L"мониторе (вкладка «Общие»).",
         L"#新機能\n"
         L"{Style}アイランド：画面上部のカプセル、MacBook のようなノッチ、固定スタイル、または全幅のバー（「スタイル」タブ）。\n"
         L"{OwnNotifications}通知がカプセルからバナーとして現れ、返信もでき、ベルが一覧を保持します（「通知」タブ）。\n"
         L"コントロール センター（カプセルのアイコン）：Wi-Fi、Bluetooth、アプリごとの音、明るさ、応答不可、夜間モード。\n"
         L"再生中、カレンダーと予定、天気、メモ、タスク、フォルダー、壁紙など、それぞれオプションで。\n"
         L"{Dock}macOS のような Dock、または透明にした Windows タスクバー（Windhawk の設定）。\n"
         L"{Profiles}プロファイル、バックアップ、自分の色、モニターごとのアイランド（「全般」タブ）。",
         L"#Tính năng mới\n"
         L"{Style}Đảo: thanh nổi ở cạnh trên màn hình, tai thỏ như MacBook, kiểu Cố định hoặc một thanh ngang toàn chiều rộng (tab Kiểu dáng).\n"
         L"{OwnNotifications}Thông báo rơi ra từ thanh đảo dưới dạng thông báo nổi, có trả lời, và chuông giữ danh sách (tab Thông báo).\n"
         L"Trung tâm điều khiển (icon của nó trên thanh đảo): Wi-Fi, Bluetooth, âm thanh từng ứng dụng, độ sáng, Không làm phiền và Ánh sáng đêm.\n"
         L"Đang phát, lịch kèm các lịch đăng ký của bạn, thời tiết, ghi chú, công việc, thư mục, hình nền và nhiều hơn nữa, mỗi thứ là một tùy chọn.\n"
         L"{Dock}Dock như của macOS, hoặc taskbar Windows được làm trong suốt (cài đặt của Windhawk).\n"
         L"{Profiles}Hồ sơ, sao lưu, màu riêng của bạn và một đảo trên mỗi màn hình (tab Chung)."}},
};
constexpr int kReleaseNotesCount = (int)std::size(kReleaseNotes);
// The language they show in there (a TextLanguage; -1: the island's own,
// as each time the page opens).
int g_notesLanguage = -1;

TextLanguage IslandTextLanguage() {
    if (g_textLanguage >= 1 && g_textLanguage <= kTextLanguages) {
        return (TextLanguage)(g_textLanguage - 1);
    }
    static const TextLanguage windows = [] {
        const LANGID id = GetUserDefaultUILanguage();
        switch (PRIMARYLANGID(id)) {
            case LANG_PORTUGUESE:
                return TextLanguage::Portuguese;
            case LANG_CHINESE:
                return SUBLANGID(id) == SUBLANG_CHINESE_SIMPLIFIED ||
                               SUBLANGID(id) == SUBLANG_CHINESE_SINGAPORE
                           ? TextLanguage::Chinese
                           : TextLanguage::English;
            case LANG_GERMAN:
                return TextLanguage::German;
            case LANG_SPANISH:
                return TextLanguage::Spanish;
            case LANG_RUSSIAN:
                return TextLanguage::Russian;
            case LANG_JAPANESE:
                return TextLanguage::Japanese;
            case LANG_VIETNAMESE:
                return TextLanguage::Vietnamese;
        }
        return TextLanguage::English;
    }();
    return windows;
}

// The names in dates (days, months, AM or PM) in the island's language when
// one is chosen; otherwise, and for the rest (24 hours, the week's first
// day, units), the Windows regional format.
PCWSTR DateLocale() {
    switch (g_textLanguage) {
        case 1:
            return L"en-US";
        case 2:
            return L"pt-BR";
        case 3:
            return L"zh-CN";
        case 4:
            return L"de-DE";
        case 5:
            return L"es-ES";
        case 6:
            return L"ru-RU";
        case 7:
            return L"ja-JP";
        case 8:
            return L"vi-VN";
    }
    return LOCALE_NAME_USER_DEFAULT;
}

// The day and the month in the order the dates' language writes them
// ("October 18", "18 de outubro" without its words, "10月18日"), with the
// month's name short or whole.
std::wstring MonthDayPicture(bool shortMonth) {
    std::wstring kept;
    WCHAR pattern[64];
    if (GetLocaleInfoEx(DateLocale(), LOCALE_SMONTHDAY, pattern,
                        ARRAYSIZE(pattern))) {
        bool quoted = false;
        for (PCWSTR c = pattern; *c; c++) {
            if (*c == L'\'') {
                quoted = !quoted;
            } else if (!quoted &&
                       (*c != L' ' || (!kept.empty() && kept.back() != L' '))) {
                kept += *c;
            }
        }
        while (!kept.empty() && kept.back() == L' ') {
            kept.pop_back();
        }
    }
    if (kept.find(L'd') == std::wstring::npos ||
        kept.find(L'M') == std::wstring::npos) {
        return shortMonth ? L"d MMM" : L"d MMMM";
    }
    if (const size_t month = kept.find(L"MMMM");
        shortMonth && month != std::wstring::npos) {
        kept.erase(month, 1);
    }
    return kept;
}

// `english`'s text in a table of another language (sorted by the English
// ones), or `english` without one there.
template <size_t N>
PCWSTR TableText(const std::pair<PCWSTR, PCWSTR> (&table)[N],
                 PCWSTR english) {
    const auto end = std::end(table);
    const auto it = std::lower_bound(
        std::begin(table), end, english,
        [](const std::pair<PCWSTR, PCWSTR>& entry, PCWSTR text) {
            return wcscmp(entry.first, text) < 0;
        });
    return it != end && wcscmp(it->first, english) == 0 ? it->second
                                                         : english;
}

// The island's own texts, in English, Portuguese or one of the other
// languages' tables (a text without its translation stays in English).
PCWSTR Tr(PCWSTR english, PCWSTR portuguese) {
    switch (IslandTextLanguage()) {
        case TextLanguage::Portuguese:
            return portuguese;
        case TextLanguage::Chinese:
            return TableText(kChineseTexts, english);
        case TextLanguage::German:
            return TableText(kGermanTexts, english);
        case TextLanguage::Spanish:
            return TableText(kSpanishTexts, english);
        case TextLanguage::Russian:
            return TableText(kRussianTexts, english);
        case TextLanguage::Japanese:
            return TableText(kJapaneseTexts, english);
        case TextLanguage::Vietnamese:
            return TableText(kVietnameseTexts, english);
        default:
            return english;
    }
}

// Presses Windows+key, to open a Windows panel: A for the quick settings, N
// for the notifications.
void PressWindowsKey(WORD key) {
    INPUT inputs[4] = {};
    for (INPUT& input : inputs) {
        input.type = INPUT_KEYBOARD;
    }
    inputs[0].ki.wVk = VK_LWIN;
    inputs[1].ki.wVk = key;
    inputs[2].ki.wVk = key;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].ki.wVk = VK_LWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

// Presses keys together (a shortcut), then lets them go in reverse order.
void PressKeys(std::initializer_list<WORD> keys) {
    std::vector<INPUT> inputs;
    for (WORD key : keys) {
        INPUT input{INPUT_KEYBOARD};
        input.ki.wVk = key;
        inputs.push_back(input);
    }
    for (auto it = std::rbegin(keys); it != std::rend(keys); ++it) {
        INPUT input{INPUT_KEYBOARD};
        input.ki.wVk = *it;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(input);
    }
    SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
}

////////////////////////////////////////////////////////////////////////////////
// Night light. Windows has no API for it: it keeps its state in the registry
// (CloudStore) as a small binary record, and applies a new record written there,
// which is how scripts and tools switch it. The record is checked strictly
// before changing anything; if it doesn't look as expected, the mod leaves it
// alone and the control center shows the tile as unavailable. There's a record
// for the whole account and, on newer builds, one per device; both are kept the
// same.
//
// The record: 43 42 01 00 0A 02 01 00 2A 06 <time, varint> 2A 2B 0E <length>
// then the state, 43 42 01 00 [10 00 when on] ..., then 00 00 00 (00).

constexpr WCHAR kCloudStoreKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\"
    L"DefaultAccount\\Current";
constexpr WCHAR kNightLightState[] =
    L"windows.data.bluelightreduction.bluelightreductionstate";

struct NightLightRecord {
    // The value's key, under kCloudStoreKey.
    std::wstring key;
    std::vector<BYTE> data;
    size_t stampAt = 0;
    size_t stampLength = 0;
    size_t lengthAt = 0;
    bool on = false;
};

bool ParseNightLight(NightLightRecord& record) {
    static const BYTE kHeader[] = {0x43, 0x42, 0x01, 0x00, 0x0A,
                                   0x02, 0x01, 0x00, 0x2A, 0x06};
    static const BYTE kState[] = {0x43, 0x42, 0x01, 0x00};
    const auto& d = record.data;
    if (d.size() < 30 || memcmp(d.data(), kHeader, sizeof(kHeader)) != 0) {
        return false;
    }
    size_t i = sizeof(kHeader);
    record.stampAt = i;
    while (i < d.size() && (d[i] & 0x80) && i - record.stampAt < 9) {
        i++;
    }
    if (i >= d.size() || (d[i] & 0x80)) {
        return false;
    }
    i++;
    record.stampLength = i - record.stampAt;
    if (i + 4 > d.size() || d[i] != 0x2A || d[i + 1] != 0x2B ||
        d[i + 2] != 0x0E) {
        return false;
    }
    record.lengthAt = i + 3;
    const size_t state = record.lengthAt + 1;
    if (state + d[record.lengthAt] > d.size() || d[record.lengthAt] < 6 ||
        memcmp(&d[state], kState, sizeof(kState)) != 0) {
        return false;
    }
    record.on = d[state + 4] == 0x10 && d[state + 5] == 0x00;
    return true;
}

// The records there are, read and checked.
std::vector<NightLightRecord> ReadNightLightRecords() {
    std::vector<NightLightRecord> records;
    HKEY store;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, kCloudStoreKey, 0, KEY_READ,
                     &store) != ERROR_SUCCESS) {
        return records;
    }
    const std::wstring_view state{kNightLightState};
    WCHAR name[256];
    for (DWORD index = 0;; index++) {
        DWORD length = ARRAYSIZE(name);
        if (RegEnumKeyEx(store, index, name, &length, nullptr, nullptr,
                         nullptr, nullptr) != ERROR_SUCCESS) {
            break;
        }
        // "<scope>$windows...state" or "<device>$windows...stateperdevice".
        const std::wstring_view key{name, length};
        const size_t dollar = key.find(L'$');
        if (dollar == std::wstring_view::npos) {
            continue;
        }
        const std::wstring_view value = key.substr(dollar + 1);
        if (value != state && value != std::wstring(state) + L"perdevice") {
            continue;
        }
        NightLightRecord record;
        record.key = std::wstring(key) + L"\\" + std::wstring(value);
        DWORD size = 0;
        if (RegGetValue(store, record.key.c_str(), L"Data", RRF_RT_REG_BINARY,
                        nullptr, nullptr, &size) != ERROR_SUCCESS ||
            size == 0 || size > 4096) {
            continue;
        }
        record.data.resize(size);
        if (RegGetValue(store, record.key.c_str(), L"Data", RRF_RT_REG_BINARY,
                        nullptr, record.data.data(),
                        &size) == ERROR_SUCCESS &&
            ParseNightLight(record)) {
            record.data.resize(size);
            records.push_back(std::move(record));
        }
    }
    RegCloseKey(store);
    return records;
}

// 1 on, 0 off, -1 unknown (no record, or not as expected).
int GetNightLight() {
    const auto records = ReadNightLightRecords();
    if (records.empty()) {
        return -1;
    }
    // The device's own record wins.
    for (const auto& record : records) {
        if (record.key.find(L"perdevice") != std::wstring::npos) {
            return record.on;
        }
    }
    return records[0].on;
}

bool SetNightLight(bool on) {
    auto records = ReadNightLightRecords();
    bool changed = false;
    // Now, in seconds since 1970. Windows only applies a record whose stamp
    // is newer than the one it has: a second change within the same second
    // (a quick second click) gets the next second.
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);
    const ULONGLONG now = ((((ULONGLONG)fileTime.dwHighDateTime << 32) |
                            fileTime.dwLowDateTime) -
                           116444736000000000ULL) /
                          10000000;

    for (auto& record : records) {
        auto& d = record.data;
        ULONGLONG before = 0;
        for (size_t i = 0; i < record.stampLength && i < 9; i++) {
            before |= (ULONGLONG)(d[record.stampAt + i] & 0x7F) << (7 * i);
        }
        // As a varint.
        ULONGLONG value = std::max(now, before + 1);
        std::vector<BYTE> stamp;
        do {
            BYTE byte = value & 0x7F;
            value >>= 7;
            stamp.push_back(value ? byte | 0x80 : byte);
        } while (value);
        const size_t state = record.lengthAt + 1;
        if (on && !record.on) {
            d.insert(d.begin() + state + 4, {0x10, 0x00});
            d[record.lengthAt] += 2;
        } else if (!on && record.on) {
            d.erase(d.begin() + state + 4, d.begin() + state + 6);
            d[record.lengthAt] -= 2;
        }
        d.erase(d.begin() + record.stampAt,
                d.begin() + record.stampAt + record.stampLength);
        d.insert(d.begin() + record.stampAt, stamp.begin(), stamp.end());
        // Checked again before it's written.
        NightLightRecord check{record.key, d};
        if (!ParseNightLight(check) || check.on != on) {
            continue;
        }
        const std::wstring path =
            std::wstring(kCloudStoreKey) + L"\\" + record.key;
        if (RegSetKeyValue(HKEY_CURRENT_USER, path.c_str(), L"Data",
                           REG_BINARY, d.data(),
                           (DWORD)d.size()) == ERROR_SUCCESS) {
            changed = true;
        }
    }
    return changed;
}

////////////////////////////////////////////////////////////////////////////////
// The monitors' brightness, over DDC/CI (documented, dxva2; a laptop's own
// screen through WMI, see BuiltInScreen). Each call talks to the monitor and
// takes tens of milliseconds, so they run on threads of their own (see
// ControlPanel).

// Whether the external monitors are asked over DDC/CI at all
// (islandExternalBrightness). Some docks and MST hubs make a monitor flicker,
// or reset, at every call, so it can be turned off: nothing goes over the
// cable then (see ForEachPhysicalMonitor). The brightness threads read it, so
// it's an atomic the option's setter and LoadSettings write, like
// g_bannerWhenMinimized.
std::atomic<bool> g_externalBrightness{true};

// Collects the monitors (see ForEachPhysicalMonitor).
BOOL CALLBACK CollectMonitorProc(HMONITOR monitor,
                                 HDC,
                                 LPRECT,
                                 LPARAM parameter) {
    ((std::vector<HMONITOR>*)parameter)->push_back(monitor);
    return TRUE;
}

// Each physical monitor, with its place in this order (counted over all of
// them), its display monitor, and its place among that one's. Even asking
// for them talks to the monitors, so with g_externalBrightness off there are
// none (see ForEachBrightnessMonitor for the ones that don't need it).
template <typename F>
void ForEachPhysicalMonitor(F&& f) {
    if (!g_externalBrightness) {
        return;
    }
    std::vector<HMONITOR> monitors;
    EnumDisplayMonitors(
        nullptr, nullptr,
        CollectMonitorProc,
        (LPARAM)&monitors);
    int index = 0;
    for (HMONITOR monitor : monitors) {
        // (Turned off meanwhile: no more calls over the cables.)
        if (!g_externalBrightness) {
            return;
        }
        DWORD count = 0;
        if (!GetNumberOfPhysicalMonitorsFromHMONITOR(monitor, &count) ||
            !count) {
            continue;
        }
        std::vector<PHYSICAL_MONITOR> physical(count);
        if (!GetPhysicalMonitorsFromHMONITOR(monitor, count,
                                             physical.data())) {
            continue;
        }
        for (DWORD within = 0; within < count && g_externalBrightness;
             within++) {
            f(physical[within], index++, monitor, (int)within);
        }
        DestroyPhysicalMonitors(count, physical.data());
    }
}

// The displays in use (Windows' display configuration).
std::vector<DISPLAYCONFIG_PATH_INFO> ActiveDisplayPaths() {
    UINT32 pathCount = 0;
    UINT32 modeCount = 0;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount,
                                    &modeCount) != ERROR_SUCCESS) {
        return {};
    }
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
    if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(),
                           &modeCount, modes.data(),
                           nullptr) != ERROR_SUCCESS) {
        return {};
    }
    paths.resize(pathCount);
    return paths;
}

// The display device a path shows ("\\.\DISPLAY1"), empty if unknown.
std::wstring DisplayPathDevice(const DISPLAYCONFIG_PATH_INFO& path) {
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
    source.header = {DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME, sizeof(source),
                     path.sourceInfo.adapterId, path.sourceInfo.id};
    return DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS
               ? source.viewGdiDeviceName
               : L"";
}

// The monitors' names as Windows' display settings show them, each with
// the display device it shows ("\\.\DISPLAY1"; one per screen showing it).
std::vector<std::pair<std::wstring, std::wstring>> MonitorFriendlyNames() {
    std::vector<std::pair<std::wstring, std::wstring>> names;
    for (const auto& path : ActiveDisplayPaths()) {
        DISPLAYCONFIG_TARGET_DEVICE_NAME target{};
        target.header = {DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME,
                         sizeof(target), path.targetInfo.adapterId,
                         path.targetInfo.id};
        std::wstring device = DisplayPathDevice(path);
        if (!device.empty() &&
            DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS &&
            target.monitorFriendlyDeviceName[0]) {
            names.push_back(
                {std::move(device), target.monitorFriendlyDeviceName});
        }
    }
    return names;
}

// Whether a display path shows the built-in screen (a laptop's).
bool IsBuiltInPath(const DISPLAYCONFIG_PATH_INFO& path) {
    const auto technology = path.targetInfo.outputTechnology;
    return technology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL ||
           technology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED ||
           technology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED;
}

// The built-in screen (a laptop's), or null. It has no DDC/CI: its
// brightness goes through WMI, as for Windows' own slider.
HMONITOR BuiltInScreen() {
    std::wstring device;
    for (const auto& path : ActiveDisplayPaths()) {
        if (IsBuiltInPath(path)) {
            device = DisplayPathDevice(path);
            break;
        }
    }
    if (device.empty()) {
        return nullptr;
    }
    std::vector<HMONITOR> monitors;
    EnumDisplayMonitors(nullptr, nullptr, CollectMonitorProc,
                        (LPARAM)&monitors);
    for (HMONITOR monitor : monitors) {
        MONITORINFOEX info{};
        info.cbSize = sizeof(info);
        if (GetMonitorInfo(monitor, &info) &&
            _wcsicmp(info.szDevice, device.c_str()) == 0) {
            return monitor;
        }
    }
    return nullptr;
}

// A screen with HDR on: the panel's brightness (DDC/CI) no longer changes
// what the desktop looks like there, Windows' "SDR content brightness"
// (Settings > Display > HDR) does, so that is what its slider sets. It's read
// and set through the display configuration, like the monitors' names: not
// over the cable (no flicker), and for any external screen that has HDR on.
// The built-in screen keeps WMI, which is Windows' own brightness slider.

