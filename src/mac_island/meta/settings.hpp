// ==WindhawkModSettings==
/*
- taskbarMode: windows
  $name: Taskbar
  $name:pt-BR: Barra de tarefas
  $name:zh-CN: 任务栏
  $name:de-DE: Taskleiste
  $name:es: Barra de tareas
  $name:ru-RU: Панель задач
  $name:ja-JP: タスクバー
  $name:vi-VN: Thanh tác vụ (Taskbar)
  $description: What happens to the Windows taskbar. Each group below says which of these it applies to
  $description:pt-BR: O que acontece com a barra de tarefas do Windows. Cada grupo abaixo diz para quais destas opções ele vale
  $description:zh-CN: Windows 任务栏的样子。下面每个分组都会说明它适用于哪些选项
  $description:de-DE: Was mit der Windows-Taskleiste passiert. Jede Gruppe unten sagt, auf welche Optionen sie sich bezieht
  $description:es: Qué pasa con la barra de tareas de Windows. Cada grupo de abajo indica a cuáles de estas opciones se aplica
  $description:ru-RU: Что происходит с панелью задач Windows. В каждой группе ниже указано, к каким из этих вариантов она относится
  $description:ja-JP: Windows のタスクバーの動作です。以下の各グループには、どの選択肢に適用されるかが書かれています
  $description:vi-VN: Chọn kiểu hiển thị cho thanh tác vụ Windows. Mỗi nhóm tùy chỉnh bên dưới sẽ ghi rõ áp dụng cho kiểu nào
  $options:
  - dock: "Dock: compact and floating, see-through, like on macOS"
  - windowsTransparent: "Windows taskbar, transparent: only the apps, with a soft shadow, hiding itself"
  - windowsApps: "Windows taskbar, only the apps (the island shows the rest)"
  - windows: "Windows taskbar, unchanged (to use other taskbar mods)"
  $options:pt-BR:
  - dock: "Dock: compacta e flutuante, transparente, como no macOS"
  - windowsTransparent: "Barra do Windows transparente: só os apps, com uma sombra suave, se escondendo sozinha"
  - windowsApps: "Barra do Windows só com os apps (a ilha mostra o resto)"
  - windows: "Barra do Windows sem mudanças (para usar outros mods de barra)"
  $options:zh-CN:
  - dock: "程序坞：紧凑、悬浮、半透明，像 macOS 一样"
  - windowsTransparent: "Windows 任务栏，透明：只显示应用，带柔和阴影，会自动隐藏"
  - windowsApps: "Windows 任务栏，只显示应用（其余由灵动岛显示）"
  - windows: "Windows 任务栏，不做更改（可与其他任务栏 mod 一起使用）"
  $options:de-DE:
  - dock: "Dock: kompakt und schwebend, durchsichtig, wie unter macOS"
  - windowsTransparent: "Windows-Taskleiste, transparent: nur die Apps, mit weichem Schatten, blendet sich selbst aus"
  - windowsApps: "Windows-Taskleiste, nur die Apps (die Insel zeigt den Rest)"
  - windows: "Windows-Taskleiste, unverändert (für die Nutzung anderer Taskleisten-Mods)"
  $options:es:
  - dock: "Dock: compacto y flotante, transparente, como en macOS"
  - windowsTransparent: "Barra de tareas de Windows, transparente: solo las apps, con una sombra suave, se oculta sola"
  - windowsApps: "Barra de tareas de Windows, solo las apps (la isla muestra el resto)"
  - windows: "Barra de tareas de Windows, sin cambios (para usar otros mods de la barra de tareas)"
  $options:ru-RU:
  - dock: "Док: компактный и плавающий, прозрачный, как в macOS"
  - windowsTransparent: "Панель задач Windows, прозрачная: только приложения, с мягкой тенью, скрывается сама"
  - windowsApps: "Панель задач Windows, только приложения (остальное показывает остров)"
  - windows: "Панель задач Windows, без изменений (для других модов панели задач)"
  $options:ja-JP:
  - dock: "Dock: コンパクトで浮かぶ透明なバー（macOS のように）"
  - windowsTransparent: "Windows タスクバー、透明: アプリのみ、やわらかい影付きで自動的に隠れる"
  - windowsApps: "Windows タスクバー、アプリのみ（その他はアイランドが表示）"
  - windows: "Windows タスクバー、変更なし（他のタスクバー Mod 用）"
  $options:vi-VN:
  - dock: "Dock: nhỏ gọn, nổi và trong suốt giống macOS"
  - windowsTransparent: "Taskbar Windows trong suốt: chỉ hiện icon ứng dụng, có đổ bóng nhẹ và tự ẩn"
  - windowsApps: "Taskbar Windows chỉ hiện ứng dụng (đồng hồ và khay hệ thống chuyển lên đảo)"
  - windows: "Taskbar Windows mặc định, giữ nguyên (để dùng chung với các mod taskbar khác)"
- windowsHideTray: false
  $name: Hide the Windows taskbar's clock and tray (with the island)
  $name:pt-BR: Esconder o relógio e a bandeja da barra do Windows (com a ilha)
  $name:zh-CN: 隐藏 Windows 任务栏的时钟和托盘（灵动岛开启时）
  $name:de-DE: Uhr und Infobereich der Windows-Taskleiste ausblenden (mit der Insel)
  $name:es: Ocultar el reloj y la bandeja de la barra de tareas de Windows (con la isla)
  $name:ru-RU: Скрыть часы и область уведомлений панели задач Windows (вместе с островом)
  $name:ja-JP: Windows タスクバーの時計とトレイを非表示（アイランドと併用）
  $name:vi-VN: Ẩn đồng hồ và khay hệ thống trên taskbar Windows (khi bật đảo)
  $description: Only for the "Windows taskbar, unchanged" choice with the island at the top on. The island already shows the clock and the tray icons, so they don't show twice. The rest of the taskbar (and what other taskbar mods do to it) stays as it is. The other Windows taskbar choices hide them already
  $description:pt-BR: Só para a opção "Barra do Windows sem mudanças" com a ilha no topo ligada. A ilha já mostra o relógio e os ícones da bandeja, então eles não aparecem duas vezes. O resto da barra (e o que outros mods de barra fazem nela) fica como está. As outras opções da barra do Windows já escondem os dois
  $description:zh-CN: 仅适用于"Windows 任务栏，不做更改"选项，并且顶部灵动岛开启时。灵动岛已经显示了时钟和托盘图标，这样就不会重复显示。任务栏的其余部分（以及其他任务栏 mod 对它的修改）保持不变。其他 Windows 任务栏选项已经会隐藏它们
  $description:de-DE: Nur für die Auswahl "Windows-Taskleiste, unverändert" bei eingeschalteter Insel oben. Die Insel zeigt bereits Uhr und Infobereich-Symbole, damit sie nicht doppelt erscheinen. Der Rest der Taskleiste (und was andere Taskleisten-Mods damit machen) bleibt unverändert. Bei den anderen Windows-Taskleisten-Optionen sind sie schon ausgeblendet
  $description:es: Solo para la opción "Barra de tareas de Windows, sin cambios" con la isla arriba activada. La isla ya muestra el reloj y los iconos de la bandeja, así que no aparecen dos veces. El resto de la barra de tareas (y lo que otros mods le hagan) queda como está. Las otras opciones de barra de tareas de Windows ya los ocultan
  $description:ru-RU: Только для варианта «Панель задач Windows, без изменений» при включённом острове вверху. Остров уже показывает часы и значки области уведомлений, поэтому они не дублируются. Остальная панель задач (и то, что с ней делают другие моды панели задач) остаётся как есть. Другие варианты панели задач Windows их уже скрывают
  $description:ja-JP: 上部のアイランドがオンのときの「Windows タスクバー、変更なし」専用です。アイランドが時計とトレイ アイコンをすでに表示するため、二重には表示されません。タスクバーのそれ以外の部分（および他のタスクバー Mod による変更）はそのままです。他の Windows タスクバーの選択肢では、すでに非表示になります
  $description:vi-VN: Chỉ dành cho tùy chọn "Taskbar Windows mặc định, giữ nguyên" khi đang bật đảo ở cạnh trên. Vì đảo đã hiện sẵn đồng hồ và icon khay hệ thống nên ẩn đi để khỏi bị trùng. Phần còn lại của taskbar (cùng hiệu ứng từ các mod khác) vẫn giữ nguyên. Các chế độ taskbar khác đã tự ẩn sẵn
- showIsland: true
  $name: Island at the top
  $name:pt-BR: Ilha no topo
  $name:zh-CN: 顶部灵动岛
  $name:de-DE: Insel oben
  $name:es: Isla arriba
  $name:ru-RU: Остров вверху
  $name:ja-JP: 上部のアイランド
  $name:vi-VN: Thanh đảo ở cạnh trên
  $description: The clock, the control center, notifications, what's playing and the other apps' tray icons, at the top of the screen. Its own options (style, colors, what it shows) are in it (right-click the island, then click the gear that comes out under it)
  $description:pt-BR: O relógio, a central de controle, as notificações, o que está tocando e os ícones dos outros apps, no topo da tela. As opções dela (estilo, cores, o que ela mostra) ficam nela mesma (clique com o botão direito na ilha e depois na engrenagem que sai embaixo dela)
  $description:zh-CN: 时钟、控制中心、通知、正在播放的内容和其他应用的托盘图标，显示在屏幕顶部。它自己的选项（样式、颜色、显示内容）在它里面（右键点击灵动岛，再点击下方出现的齿轮）
  $description:de-DE: Die Uhr, das Kontrollzentrum, Benachrichtigungen, die Wiedergabe und die Infobereich-Symbole anderer Apps, am oberen Bildschirmrand. Ihre eigenen Optionen (Stil, Farben, Anzeigen) stehen in ihr selbst (Rechtsklick auf die Insel, dann auf das Zahnrad klicken, das darunter erscheint)
  $description:es: El reloj, el centro de control, las notificaciones, lo que suena y los iconos de la bandeja de otras apps, en la parte superior de la pantalla. Sus propias opciones (estilo, colores, qué muestra) están en ella misma (clic derecho en la isla y luego clic en el engranaje que sale debajo)
  $description:ru-RU: Часы, центр управления, уведомления, воспроизводимое и значки области уведомлений других приложений вверху экрана. Его собственные параметры (стиль, цвета, что показывать) находятся в нём (щёлкните остров правой кнопкой, затем шестерёнку, появившуюся под ним)
  $description:ja-JP: 時計、コントロール センター、通知、再生中のもの、他のアプリのトレイ アイコンを画面の上部に表示します。専用のオプション（スタイル、色、表示内容）はアイランド自体にあります（アイランドを右クリックし、下に表示される歯車をクリック）
  $description:vi-VN: Hiển thị đồng hồ, trung tâm điều khiển, thông báo, nhạc đang phát và khay hệ thống ở cạnh trên màn hình. Để mở cài đặt chi tiết của đảo (kiểu dáng, màu sắc, tiện ích), hãy bấm chuột phải vào đảo rồi bấm biểu tượng bánh răng hiện ra bên dưới
- calendarIcs: [""]
  $name: Calendar agendas (.ics)
  $name:pt-BR: Agendas do calendário (.ics)
  $name:zh-CN: 日历订阅（.ics）
  $name:de-DE: Kalender-Abos (.ics)
  $name:es: Agendas del calendario (.ics)
  $name:ru-RU: Календари (.ics)
  $name:ja-JP: カレンダーの登録 (.ics)
  $name:vi-VN: Lịch đăng ký (.ics)
  $description: Links (https) or paths of .ics calendars, like Google Calendar's "Secret address in iCal format". Their events show in the island's calendar, read only, in each agenda's color (chosen in the calendar, where agendas can also be added)
  $description:pt-BR: Links (https) ou caminhos de agendas .ics, como o "Endereço secreto no formato iCal" do Google Agenda. Os eventos aparecem no calendário da ilha, só para leitura, na cor de cada agenda (escolhida no calendário, onde também dá para adicionar agendas)
  $description:zh-CN: .ics 日历的链接（https）或路径，例如 Google 日历的“iCal 格式的私密地址”。其中的事件以只读方式显示在灵动岛的日历中，使用各自的颜色（在日历中选择，也可以在日历中添加订阅）
  $description:de-DE: Links (https) oder Pfade von .ics-Kalendern, wie die "Geheime Adresse im iCal-Format" von Google Kalender. Ihre Termine erscheinen im Kalender der Insel, nur lesbar, in der Farbe des jeweiligen Kalenders (im Kalender wählbar, wo sich auch Kalender hinzufügen lassen)
  $description:es: Enlaces (https) o rutas de calendarios .ics, como la "Dirección secreta en formato iCal" de Google Calendar. Sus eventos aparecen en el calendario de la isla, solo lectura, con el color de cada agenda (se elige en el calendario, donde también se pueden añadir agendas)
  $description:ru-RU: Ссылки (https) или пути к календарям .ics, например «Закрытый адрес в формате iCal» Google Календаря. Их события показываются в календаре острова только для чтения, цветом каждого календаря (выбирается в календаре, где их можно и добавить)
  $description:ja-JP: .ics カレンダーのリンク (https) またはパス（Google カレンダーの「iCal 形式の非公開アドレス」など）。そのイベントはアイランドのカレンダーに読み取り専用で、カレンダーごとの色で表示されます（色の選択と追加はカレンダー内で行います）
  $description:vi-VN: Dán link (https) hoặc đường dẫn tệp lịch .ics, ví dụ "Địa chỉ bí mật ở định dạng iCal" của Google Calendar. Các sự kiện sẽ hiện trong lịch của đảo (chỉ đọc) với màu sắc riêng cho từng lịch (có thể thêm lịch và chọn màu trực tiếp trong bảng lịch)
- hiding:
  - autoHide: false
    $name: Hide the taskbar until the mouse reaches the bottom edge
    $name:pt-BR: Esconder a barra até o mouse chegar na borda de baixo
    $name:zh-CN: 隐藏任务栏，直到鼠标移到底部边缘
    $name:de-DE: Taskleiste ausblenden, bis die Maus den unteren Rand erreicht
    $name:es: Ocultar la barra de tareas hasta que el ratón llegue al borde inferior
    $name:ru-RU: Скрывать панель задач, пока мышь не достигнет нижнего края
    $name:ja-JP: マウスが下端に届くまでタスクバーを隠す
    $name:vi-VN: Tự ẩn thanh tác vụ cho tới khi đưa chuột xuống cạnh dưới
    $description: Turns on the Windows auto-hide while the mod is on, and back off when it's turned off (if it was off before), with every choice of "Taskbar" (off by default). The other options of this group need it
    $description:pt-BR: Liga o ocultar automaticamente do Windows enquanto o mod está ativo, e desliga de novo quando ele é desativado (se estava desligado antes), com qualquer opção de "Barra de tarefas" (desligado por padrão). As outras opções deste grupo precisam dela
    $description:zh-CN: mod 启用时打开 Windows 的自动隐藏，mod 关闭时再关掉（如果之前是关闭的），适用于"任务栏"的所有选项（默认关闭）。本组的其他选项需要它
    $description:de-DE: Schaltet das automatische Ausblenden von Windows ein, solange der Mod läuft, und wieder aus, wenn er beendet wird (falls es vorher aus war), bei jeder Auswahl von "Taskleiste" (standardmäßig aus). Die anderen Optionen dieser Gruppe setzen es voraus
    $description:es: Activa el ocultado automático de Windows mientras el mod está activo y lo desactiva al apagarlo (si estaba desactivado antes), con cada opción de "Barra de tareas" (desactivado por defecto). Las demás opciones de este grupo lo necesitan
    $description:ru-RU: Включает автоскрытие Windows, пока мод включён, и снова выключает при его отключении (если раньше было выключено), для любого варианта «Панель задач» (по умолчанию выключено). Остальные параметры этой группы требуют его
    $description:ja-JP: Mod がオンの間は Windows の自動非表示をオンにし、Mod をオフにすると元に戻します（以前オフだった場合）。「タスクバー」のどの選択肢でも使えます（既定ではオフ）。このグループの他のオプションにはこれが必要です
    $description:vi-VN: Tự bật chế độ tự động ẩn taskbar của Windows khi dùng mod và trả lại như cũ khi tắt mod (mặc định tắt). Hoạt động với mọi kiểu "Thanh tác vụ", và là điều kiện bắt buộc cho các tùy chọn bên dưới
  - hideWhen: mouseAway
    $name: When it hides
    $name:pt-BR: Quando esconder
    $name:zh-CN: 何时隐藏
    $name:de-DE: Wann sie sich ausblendet
    $name:es: Cuándo se oculta
    $name:ru-RU: Когда скрывать
    $name:ja-JP: 隠すタイミング
    $name:vi-VN: Khi nào ẩn
    $description: "With \"Mouse away\" (the default) it hides as soon as the mouse leaves it. The others keep it shown on the desktop and hide it only while an app's window is maximized on its monitor, or is over it (maximized ones too); it comes back once the window goes. The mouse at the bottom edge still brings it up. Only for \"Dock\" and \"Windows taskbar, transparent\" (and \"only the apps\" with its option in App icons)"
    $description:pt-BR: "Com \"Mouse longe\" (o padrão) ela se esconde assim que o mouse sai dela. As outras a deixam aparecendo na área de trabalho e só a escondem enquanto a janela de um app está maximizada no monitor dela, ou por cima dela (as maximizadas também); ela volta quando a janela sai. O mouse na borda de baixo continua trazendo ela. Só para \"Dock\" e \"Barra do Windows transparente\" (e \"só com os apps\" com a opção dela em Ícones dos apps)"
    $description:zh-CN: "选择“鼠标离开”（默认）时，鼠标一离开它就隐藏。其他选项让它在桌面上保持显示，只有当某个应用窗口在它所在的显示器上最大化，或盖住它（包括最大化的窗口）时才隐藏；窗口离开后它会回来。鼠标移到底部边缘仍可让它出现。仅适用于\"程序坞\"和\"Windows 任务栏，透明\"（在应用图标中开启其选项时也适用于\"只显示应用\"）"
    $description:de-DE: "Mit \"Maus weg\" (Standard) blendet sie sich aus, sobald die Maus sie verlässt. Die anderen lassen sie auf dem Desktop sichtbar und blenden sie nur aus, solange ein App-Fenster auf ihrem Monitor maximiert ist oder über ihr liegt (maximierte auch); sie kommt zurück, sobald das Fenster weg ist. Die Maus am unteren Rand blendet sie weiterhin ein. Nur für \"Dock\" und \"Windows-Taskleiste, transparent\" (und \"nur die Apps\" mit ihrer Option unter App-Symbole)"
    $description:es: "Con \"Ratón lejos\" (el predeterminado) se oculta en cuanto el ratón sale de ella. Las otras la mantienen visible en el escritorio y solo la ocultan mientras la ventana de una app está maximizada en su monitor, o encima de ella (las maximizadas también); vuelve cuando la ventana se va. El ratón en el borde inferior la sigue mostrando. Solo para \"Dock\" y \"Barra de tareas de Windows, transparente\" (y \"solo las apps\" con su opción en Iconos de apps)"
    $description:ru-RU: "При «Мышь далеко» (по умолчанию) она скрывается, как только мышь уходит с неё. Остальные варианты оставляют её видимой на рабочем столе и скрывают, только пока окно приложения развёрнуто на её мониторе или находится над ней (развёрнутые тоже); она возвращается, когда окно уходит. Мышь у нижнего края по-прежнему вызывает её. Только для «Док» и «Панель задач Windows, прозрачная» (и «только приложения» с её параметром в «Значки приложений»)"
    $description:ja-JP: "「マウスが離れたとき」（既定）では、マウスが離れるとすぐに隠れます。ほかの選択肢では、デスクトップでは表示したままにし、そのモニターでアプリのウィンドウが最大化されているとき、またはウィンドウが重なっているとき（最大化も含む）だけ隠れます。ウィンドウがなくなると戻ります。下端のマウスでは引き続き表示されます。「Dock」と「Windows タスクバー、透明」のみ（アプリのアイコンでオプションをオンにすると「アプリのみ」も）"
    $description:vi-VN: Với "Chuột rời đi" (mặc định), nó ẩn ngay khi chuột rời khỏi nó. Các mức khác giữ nó hiện trên màn hình nền và chỉ ẩn khi cửa sổ của một ứng dụng đang phóng to trên màn hình của nó, hoặc đè lên nó (cả cửa sổ phóng to); nó trở lại khi cửa sổ rời đi. Chuột ở cạnh dưới vẫn gọi nó lên. Chỉ dành cho "Dock" và "Taskbar Windows trong suốt" (và "chỉ hiện ứng dụng" khi bật tùy chọn của nó trong Biểu tượng ứng dụng)
    $options:
    - mouseAway: Mouse away
    - maximized: Window maximized
    - windowOver: Window over it
    $options:pt-BR:
    - mouseAway: Mouse longe
    - maximized: Janela maximizada
    - windowOver: Janela por cima
    $options:zh-CN:
    - mouseAway: 鼠标离开
    - maximized: 窗口最大化
    - windowOver: 窗口挡住它
    $options:de-DE:
    - mouseAway: Maus weg
    - maximized: Fenster maximiert
    - windowOver: Fenster darüber
    $options:es:
    - mouseAway: Ratón lejos
    - maximized: Ventana maximizada
    - windowOver: Ventana encima
    $options:ru-RU:
    - mouseAway: Мышь далеко
    - maximized: Окно развёрнуто
    - windowOver: Окно над ней
    $options:ja-JP:
    - mouseAway: マウスが離れたとき
    - maximized: ウィンドウの最大化
    - windowOver: ウィンドウが重なったとき
    $options:vi-VN:
    - mouseAway: Chuột rời đi
    - maximized: Cửa sổ phóng to
    - windowOver: Cửa sổ đè lên
  - showDelay: 0
    $name: Wait before showing (ms)
    $name:pt-BR: Espera para aparecer (ms)
    $name:zh-CN: 显示前等待（毫秒）
    $name:de-DE: Wartezeit vor dem Einblenden (ms)
    $name:es: Espera antes de mostrar (ms)
    $name:ru-RU: Задержка перед показом (мс)
    $name:ja-JP: 表示までの待ち時間 (ms)
    $name:vi-VN: Độ trễ trước khi hiện (ms)
    $description: How long the mouse has to stay at the bottom edge (0-2000, default 0)
    $description:pt-BR: Quanto tempo o mouse precisa ficar na borda de baixo (0-2000, padrão 0)
    $description:zh-CN: 鼠标需要在底部边缘停留多久（0-2000，默认 0）
    $description:de-DE: Wie lange die Maus am unteren Rand bleiben muss (0-2000, Standard 0)
    $description:es: Cuánto tiempo debe quedarse el ratón en el borde inferior (0-2000, predeterminado 0)
    $description:ru-RU: Как долго мышь должна находиться у нижнего края (0-2000, по умолчанию 0)
    $description:ja-JP: マウスが下端にとどまる必要がある時間（0-2000、既定値 0）
    $description:vi-VN: Thời gian cần giữ chuột ở sát cạnh dưới để gọi thanh tác vụ lên (0-2000, mặc định 0)
  - revealHeight: 0
    $name: Height over the bottom edge that brings it up (px at 100% scale)
    $name:pt-BR: Altura acima da borda de baixo que faz ela aparecer (px na escala de 100%)
    $name:zh-CN: 底部边缘上方触发显示的高度（像素，按 100% 缩放）
    $name:de-DE: Höhe über dem unteren Rand, die sie einblendet (px bei 100 % Skalierung)
    $name:es: Altura sobre el borde inferior que la muestra (px con escala al 100 %)
    $name:ru-RU: Высота над нижним краем, при которой она появляется (пикс. при масштабе 100%)
    $name:ja-JP: 表示のきっかけになる下端からの高さ (px、100% スケール時)
    $name:vi-VN: Độ cao vùng nhận diện chuột ở cạnh dưới (px ở tỷ lệ 100%)
    $description: The same on every monitor. 0 is automatic (the default), only the edge itself, and the icons' height on a monitor with another one below it (where the mouse easily goes past the edge). Only for "Dock" and "Windows taskbar, transparent" (0-120, default 0)
    $description:pt-BR: A mesma em todos os monitores. 0 é automático (o padrão), só a própria borda, e a altura dos ícones num monitor com outro embaixo dele (onde o mouse passa fácil da borda). Só para "Dock" e "Barra do Windows transparente" (0-120, padrão 0)
    $description:zh-CN: 所有显示器相同。0 为自动（默认）：只有边缘本身；下方还有另一台显示器时为图标的高度（鼠标很容易越过边缘）。仅适用于"程序坞"和"Windows 任务栏，透明"（0-120，默认 0）
    $description:de-DE: "Gleich auf jedem Monitor. 0 ist automatisch, der Standard: nur der Rand selbst und die Symbolhöhe auf einem Monitor mit einem weiteren darunter (wo die Maus leicht über den Rand hinausgeht). Nur für \"Dock\" und \"Windows-Taskleiste, transparent\" (0-120, Standard 0)"
    $description:es: "Igual en todos los monitores. 0 es automático, el valor predeterminado: solo el borde en sí, y la altura de los iconos en un monitor con otro debajo (donde el ratón se pasa fácilmente del borde). Solo para \"Dock\" y \"Barra de tareas de Windows, transparente\" (0-120, predeterminado 0)"
    $description:ru-RU: "Одинаково на всех мониторах. 0 — автоматически, по умолчанию: только сам край, а на мониторе с другим монитором снизу (где мышь легко проскакивает край) — высота значков. Только для «Док» и «Панель задач Windows, прозрачная» (0-120, по умолчанию 0)"
    $description:ja-JP: "すべてのモニターで共通です。0 は自動（既定）: 端そのもののみ。下に別のモニターがある場合（マウスが端を通り過ぎやすい）はアイコンの高さになります。「Dock」と「Windows タスクバー、透明」のみ（0-120、既定値 0）"
    $description:vi-VN: "Áp dụng như nhau trên mọi màn hình. Đặt 0 là tự động (mặc định): chỉ nhận sát mép dưới cùng, hoặc lấy bằng chiều cao icon nếu bên dưới còn một màn hình nữa (giúp chuột không bị lố qua mép). Chỉ áp dụng cho \"Dock\" và \"Taskbar Windows trong suốt\" (0-120, mặc định 0)"
  - hideDelay: 400
    $name: Wait before hiding (ms)
    $name:pt-BR: Espera para esconder (ms)
    $name:zh-CN: 隐藏前等待（毫秒）
    $name:de-DE: Wartezeit vor dem Ausblenden (ms)
    $name:es: Espera antes de ocultar (ms)
    $name:ru-RU: Задержка перед скрытием (мс)
    $name:ja-JP: 非表示までの待ち時間 (ms)
    $name:vi-VN: Độ trễ trước khi ẩn (ms)
    $description: How long it stays after the mouse leaves it (0-5000, default 400)
    $description:pt-BR: Quanto tempo ela fica depois que o mouse sai (0-5000, padrão 400)
    $description:zh-CN: 鼠标离开后它保留多久（0-5000，默认 400）
    $description:de-DE: Wie lange sie bleibt, nachdem die Maus sie verlassen hat (0-5000, Standard 400)
    $description:es: Cuánto tiempo se queda tras salir el ratón (0-5000, predeterminado 400)
    $description:ru-RU: Как долго она остаётся после ухода мыши (0-5000, по умолчанию 400)
    $description:ja-JP: マウスが離れてから表示を続ける時間（0-5000、既定値 400）
    $description:vi-VN: Thời gian chờ sau khi đưa chuột ra ngoài trước khi tự ẩn xuống (0-5000, mặc định 400)
  - showDuration: 260
    $name: Showing animation (ms)
    $name:pt-BR: Animação de aparecer (ms)
    $name:zh-CN: 显示动画（毫秒）
    $name:de-DE: Einblendanimation (ms)
    $name:es: Animación de aparición (ms)
    $name:ru-RU: Анимация появления (мс)
    $name:ja-JP: 表示アニメーション (ms)
    $name:vi-VN: Thời lượng hiệu ứng trượt lên (ms)
    $description: How long it takes to slide in, with a little spring; 0 is instant, and the speed in the animations group changes it too (0-1000, default 260)
    $description:pt-BR: Quanto tempo ela leva para entrar, com um leve efeito de mola; 0 é na hora, e a velocidade do grupo de animações também muda isso (0-1000, padrão 260)
    $description:zh-CN: 滑入所需的时间，带一点弹性；0 为立即，动画分组中的速度也会影响它（0-1000，默认 260）
    $description:de-DE: Wie lange das Hineingleiten mit leichter Federung dauert; 0 ist sofort, und die Geschwindigkeit in der Animationsgruppe ändert es ebenfalls (0-1000, Standard 260)
    $description:es: Cuánto tarda en deslizarse hacia dentro, con un pequeño rebote; 0 es instantáneo, y la velocidad del grupo de animaciones también lo cambia (0-1000, predeterminado 260)
    $description:ru-RU: Сколько длится выезд с лёгким пружинным эффектом; 0 — мгновенно, скорость в группе анимаций тоже влияет (0-1000, по умолчанию 260)
    $description:ja-JP: わずかなバネ効果でスライドインする時間。0 は即時で、アニメーション グループの速度にも影響されます（0-1000、既定値 260）
    $description:vi-VN: Thời gian trượt lên kèm độ nảy nhẹ; đặt 0 để hiện tức thì. Chịu ảnh hưởng thêm bởi mục Tốc độ ở nhóm chuyển động bên dưới (0-1000, mặc định 260)
  - hideDuration: 220
    $name: Hiding animation (ms)
    $name:pt-BR: Animação de esconder (ms)
    $name:zh-CN: 隐藏动画（毫秒）
    $name:de-DE: Ausblendanimation (ms)
    $name:es: Animación de ocultación (ms)
    $name:ru-RU: Анимация скрытия (мс)
    $name:ja-JP: 非表示アニメーション (ms)
    $name:vi-VN: Thời lượng hiệu ứng trượt xuống (ms)
    $description: How long it takes to slide out; 0 is instant (0-1000, default 220)
    $description:pt-BR: Quanto tempo ela leva para sair; 0 é na hora (0-1000, padrão 220)
    $description:zh-CN: 滑出所需的时间；0 为立即（0-1000，默认 220）
    $description:de-DE: Wie lange das Hinausgleiten dauert; 0 ist sofort (0-1000, Standard 220)
    $description:es: Cuánto tarda en deslizarse hacia fuera; 0 es instantáneo (0-1000, predeterminado 220)
    $description:ru-RU: Сколько длится уход; 0 — мгновенно (0-1000, по умолчанию 220)
    $description:ja-JP: スライドアウトする時間。0 は即時（0-1000、既定値 220）
    $description:vi-VN: Thời gian trượt xuống khi ẩn đi; đặt 0 để ẩn tức thì (0-1000, mặc định 220)
  - fullscreenGuard: true
    $name: Not over games and videos in full screen
    $name:pt-BR: Não aparecer sobre jogos e vídeos em tela cheia
    $name:zh-CN: 不在全屏游戏和视频上方出现
    $name:de-DE: Nicht über Spielen und Videos im Vollbild
    $name:es: No sobre juegos ni vídeos en pantalla completa
    $name:ru-RU: Не поверх игр и видео на весь экран
    $name:ja-JP: 全画面のゲームや動画の上には表示しない
    $name:vi-VN: Không tự bung lên khi đang chơi game hoặc xem video toàn màn hình
    $description: The mouse at the bottom edge doesn't bring it up over them (the Windows key still does)
    $description:pt-BR: O mouse na borda de baixo não faz ela aparecer sobre eles (a tecla Windows continua funcionando)
    $description:zh-CN: 鼠标移到底部边缘不会让它出现在它们上方（Windows 键仍然可以）
    $description:de-DE: Die Maus am unteren Rand blendet sie darüber nicht ein (die Windows-Taste schon)
    $description:es: El ratón en el borde inferior no la muestra sobre ellos (la tecla Windows sí)
    $description:ru-RU: Мышь у нижнего края не вызывает её поверх них (клавиша Windows по-прежнему вызывает)
    $description:ja-JP: 下端にマウスを置いても、それらの上には表示されません（Windows キーでは引き続き表示されます）
    $description:vi-VN: Đưa chuột chạm cạnh dưới sẽ không làm thanh Dock hiện đè lên game hay video toàn màn hình (nhấn phím Windows thì vẫn hiện)
  $name: Hiding and showing
  $name:pt-BR: Esconder e mostrar
  $name:zh-CN: 隐藏和显示
  $name:de-DE: Aus- und Einblenden
  $name:es: Ocultar y mostrar
  $name:ru-RU: Скрытие и показ
  $name:ja-JP: 非表示と表示
  $name:vi-VN: Tự động ẩn & hiện
  $description: Hiding the taskbar is for every choice above; the waits, the animations and full screen only for "Dock" and "Windows taskbar, transparent" (the others hide the Windows way)
  $description:pt-BR: Esconder a barra vale para todas as opções acima; as esperas, as animações e a tela cheia só para "Dock" e "Barra do Windows transparente" (as outras se escondem do jeito do Windows)
  $description:zh-CN: 隐藏任务栏适用于上面的所有选项；等待、动画和全屏仅适用于"程序坞"和"Windows 任务栏，透明"（其他选项按 Windows 自己的方式隐藏）
  $description:de-DE: Das Ausblenden der Taskleiste gilt für jede Auswahl oben; Wartezeiten, Animationen und Vollbild nur für "Dock" und "Windows-Taskleiste, transparent" (die anderen blenden sich nach Windows-Art aus)
  $description:es: Ocultar la barra de tareas vale para todas las opciones de arriba; las esperas, las animaciones y la pantalla completa solo para "Dock" y "Barra de tareas de Windows, transparente" (las demás se ocultan a la manera de Windows)
  $description:ru-RU: Скрытие панели задач действует для всех вариантов выше; задержки, анимации и полный экран — только для «Док» и «Панель задач Windows, прозрачная» (остальные скрываются средствами Windows)
  $description:ja-JP: タスクバーの非表示は上のすべての選択肢に適用されます。待ち時間、アニメーション、全画面は「Dock」と「Windows タスクバー、透明」のみ（他は Windows の方法で隠れます）
  $description:vi-VN: Tùy chọn tự ẩn áp dụng được cho mọi kiểu taskbar; riêng độ trễ, hiệu ứng trượt và chặn toàn màn hình chỉ dành cho "Dock" và "Taskbar Windows trong suốt" (các kiểu còn lại ẩn theo cơ chế mặc định của Windows)
- icons:
  - magnification: 170
    $name: Icon magnification (%)
    $name:pt-BR: Ampliação dos ícones (%)
    $name:zh-CN: 图标放大（%）
    $name:de-DE: Symbolvergrößerung (%)
    $name:es: Ampliación de iconos (%)
    $name:ru-RU: Увеличение значков (%)
    $name:ja-JP: アイコンの拡大 (%)
    $name:vi-VN: Độ phóng đại icon khi rê chuột (%)
    $description: How big the icon under the mouse gets, in a wave like on macOS, pushing its neighbors aside; 100 turns it off. The dock makes room above itself for them; on the transparent taskbar they only grow up to its top (100-200, default 170)
    $description:pt-BR: O quanto o ícone embaixo do mouse cresce, numa onda como no macOS, afastando os vizinhos; 100 desliga. A dock abre espaço acima dela para eles; na barra transparente eles só crescem até o topo dela (100-200, padrão 170)
    $description:zh-CN: 鼠标下的图标放大多少，像 macOS 一样呈波浪状，并把相邻图标推开；100 为关闭。程序坞会在上方留出空间；在透明任务栏上图标最多只能放大到任务栏顶部（100-200，默认 170）
    $description:de-DE: Wie groß das Symbol unter der Maus wird, in einer Welle wie unter macOS, wobei die Nachbarn zur Seite rücken; 100 schaltet es aus. Das Dock schafft darüber Platz dafür; in der transparenten Taskleiste wachsen sie nur bis zu ihrer Oberkante (100-200, Standard 170)
    $description:es: Cuánto crece el icono bajo el ratón, en una onda como en macOS, apartando a sus vecinos; 100 lo desactiva. El dock deja espacio encima para ellos; en la barra de tareas transparente solo crecen hasta su borde superior (100-200, predeterminado 170)
    $description:ru-RU: Насколько увеличивается значок под мышью волной, как в macOS, раздвигая соседние; 100 — выключено. Док освобождает над собой место для них; на прозрачной панели задач они вырастают только до её верха (100-200, по умолчанию 170)
    $description:ja-JP: マウスの下のアイコンが macOS のように波状に拡大し、隣のアイコンを押し広げる大きさ。100 でオフ。Dock は上に余白を作ります。透明タスクバーでは、上端までしか拡大しません（100-200、既定値 170）
    $description:vi-VN: Phóng to icon dưới trỏ chuột theo hiệu ứng lượn sóng giống macOS và đẩy nhẹ các icon bên cạnh ra; đặt 100 để tắt. Thanh Dock sẽ tự mở rộng không gian phía trên cho icon phóng to, còn taskbar trong suốt chỉ phóng to tối đa bằng chiều cao taskbar (100-200, mặc định 170)
  - magnificationRange: 150
    $name: Magnification reach (px)
    $name:pt-BR: Alcance da ampliação (px)
    $name:zh-CN: 放大范围（像素）
    $name:de-DE: Reichweite der Vergrößerung (px)
    $name:es: Alcance de la ampliación (px)
    $name:ru-RU: Зона увеличения (px)
    $name:ja-JP: 拡大の範囲 (px)
    $name:vi-VN: Bán kính lan hiệu ứng sóng (px)
    $description: How far from the mouse the neighboring icons still grow. Only with magnification above 100 (40-300, default 150)
    $description:pt-BR: Até que distância do mouse os ícones vizinhos também crescem. Só com ampliação acima de 100 (40-300, padrão 150)
    $description:zh-CN: 距离鼠标多远的相邻图标也会放大。仅在放大大于 100 时有效（40-300，默认 150）
    $description:de-DE: Wie weit von der Maus entfernt die Nachbarsymbole noch wachsen. Nur bei Vergrößerung über 100 (40-300, Standard 150)
    $description:es: A qué distancia del ratón siguen creciendo los iconos vecinos. Solo con ampliación superior a 100 (40-300, predeterminado 150)
    $description:ru-RU: На каком расстоянии от мыши соседние значки ещё увеличиваются. Только при увеличении больше 100 (40-300, по умолчанию 150)
    $description:ja-JP: マウスからどこまで離れた隣のアイコンまで拡大するか。拡大が 100 を超えるときのみ（40-300、既定値 150）
    $description:vi-VN: Khoảng cách tính từ trỏ chuột mà các icon lân cận vẫn được phóng to theo hiệu ứng sóng. Chỉ có tác dụng khi độ phóng đại lớn hơn 100 (40-300, mặc định 150)
  - onlyApps: false
    $name: Also on "Windows taskbar, only the apps"
    $name:pt-BR: Também na "Barra do Windows só com os apps"
    $name:zh-CN: 也用于"Windows 任务栏，只显示应用"
    $name:de-DE: Auch für "Windows-Taskleiste, nur die Apps"
    $name:es: También en "Barra de tareas de Windows, solo las apps"
    $name:ru-RU: Также для «Панель задач Windows, только приложения»
    $name:ja-JP: 「Windows タスクバー、アプリのみ」でも使う
    $name:vi-VN: Dùng cả cho "Taskbar Windows chỉ hiện ứng dụng"
    $description: The magnification and the bounces on that taskbar too (the icons grow up to its top), and "When it hides" with Windows' own auto-hide. Off, that taskbar stays as Windows has it
    $description:pt-BR: A ampliação e os pulos nessa barra também (os ícones crescem até o topo dela), e o "Quando esconder" com o esconder automático do próprio Windows. Desligado, essa barra fica como o Windows a deixa
    $description:zh-CN: 该任务栏也使用放大和跳动（图标最多放大到任务栏顶部），并在 Windows 自带的自动隐藏下使用"何时隐藏"。关闭时，该任务栏保持 Windows 的原样
    $description:de-DE: Die Vergrößerung und das Hüpfen auch in dieser Taskleiste (die Symbole wachsen bis zu ihrer Oberkante) und "Wann sie sich ausblendet" mit dem automatischen Ausblenden von Windows. Aus, bleibt diese Taskleiste, wie Windows sie hat
    $description:es: La ampliación y los rebotes también en esa barra (los iconos crecen hasta su borde superior), y "Cuándo se oculta" con la ocultación automática del propio Windows. Desactivado, esa barra queda como la tiene Windows
    $description:ru-RU: Увеличение и подпрыгивание и на этой панели (значки вырастают до её верха), а также «Когда скрывать» с автоскрытием самой Windows. Если выключено, эта панель остаётся такой, как в Windows
    $description:ja-JP: そのタスクバーでも拡大とバウンスを使い（アイコンは上端まで拡大）、Windows 自身の自動非表示で「隠すタイミング」を使います。オフのときは Windows のままです
    $description:vi-VN: Độ phóng đại và hiệu ứng nảy cũng dùng trên taskbar đó (icon lớn lên tới mép trên của nó), và "Khi nào ẩn" với tự ẩn của chính Windows. Khi tắt, taskbar đó giữ nguyên như Windows đang có
  - bounceOnLaunch: true
    $name: Bounce while an app opens
    $name:pt-BR: Pular enquanto um app abre
    $name:zh-CN: 应用打开时跳动
    $name:de-DE: Hüpfen, während eine App startet
    $name:es: Rebotar mientras se abre una app
    $name:ru-RU: Подпрыгивание при открытии приложения
    $name:ja-JP: アプリの起動中にバウンス
    $name:vi-VN: Hiệu ứng nảy icon khi mở ứng dụng
    $description: The icon of an app being opened bounces until its window shows, like on macOS (not with the "partial" or no animations)
    $description:pt-BR: O ícone de um app abrindo pula até a janela dele aparecer, como no macOS (não com animações "parciais" ou sem animações)
    $description:zh-CN: 正在打开的应用图标会一直跳动，直到窗口出现，像 macOS 一样（"部分"动画或无动画时不会）
    $description:de-DE: Das Symbol einer startenden App hüpft, bis ihr Fenster erscheint, wie unter macOS (nicht bei "Teilweise" oder ohne Animationen)
    $description:es: El icono de una app que se abre rebota hasta que aparece su ventana, como en macOS (no con "Parcial" ni sin animaciones)
    $description:ru-RU: Значок открываемого приложения подпрыгивает, пока не появится его окно, как в macOS (не при «Частично» и без анимаций)
    $description:ja-JP: 起動中のアプリのアイコンは、macOS のようにウィンドウが表示されるまでバウンスします（「一部」またはアニメーションなしのときは無効）
    $description:vi-VN: Icon của ứng dụng vừa bấm mở sẽ nảy lên xuống như macOS cho tới khi cửa sổ hiện ra (không hoạt động nếu chọn hiệu ứng "Cơ bản" hoặc tắt hiệu ứng)
  - keepOpenOnLaunch: true
    $name: Stay up while a clicked app opens
    $name:pt-BR: Ficar aberta enquanto um app clicado abre
    $name:zh-CN: 点击的应用打开时保持显示
    $name:de-DE: Oben bleiben, während eine angeklickte App startet
    $name:es: Quedarse arriba mientras se abre una app pulsada
    $name:ru-RU: Оставаться, пока открывается нажатое приложение
    $name:ja-JP: クリックしたアプリの起動中は表示したままにする
    $name:vi-VN: Giữ thanh Dock hiện trong lúc chờ ứng dụng mở lên
    $description: After clicking an app that isn't open, the taskbar waits for it to open (and bounce) before hiding. Only with the hiding of the group above
    $description:pt-BR: Depois de clicar num app que não está aberto, a barra espera ele abrir (e pular) antes de se esconder. Só com o esconder do grupo acima
    $description:zh-CN: 点击一个未打开的应用后，任务栏会等它打开（并跳动）后再隐藏。仅在上面分组的隐藏开启时有效
    $description:de-DE: Nach dem Klick auf eine nicht geöffnete App wartet die Taskleiste, bis sie startet (und hüpft), bevor sie sich ausblendet. Nur mit dem Ausblenden der Gruppe oben
    $description:es: Tras hacer clic en una app que no está abierta, la barra de tareas espera a que se abra (y rebote) antes de ocultarse. Solo con el ocultado del grupo de arriba
    $description:ru-RU: После нажатия на неоткрытое приложение панель задач ждёт его открытия (и подпрыгивания) и только потом скрывается. Только при скрытии из группы выше
    $description:ja-JP: 開いていないアプリをクリックした後、タスクバーはそのアプリが開く（とバウンスする）まで待ってから隠れます。上のグループの自動非表示がオンのときのみ
    $description:vi-VN: Sau khi bấm mở một ứng dụng, thanh Dock sẽ chờ ứng dụng khởi động xong (kèm hiệu ứng nảy icon) rồi mới tự ẩn xuống. Chỉ áp dụng khi bật tự động ẩn ở nhóm trên
  - keepOpenDuration: 1500
    $name: How long it stays after the app opens (ms)
    $name:pt-BR: Quanto tempo ela fica depois que o app abre (ms)
    $name:zh-CN: 应用打开后保持多久（毫秒）
    $name:de-DE: Wie lange sie nach dem Start der App bleibt (ms)
    $name:es: Cuánto se queda tras abrirse la app (ms)
    $name:ru-RU: Как долго остаётся после открытия приложения (мс)
    $name:ja-JP: アプリが開いてから表示を続ける時間 (ms)
    $name:vi-VN: Thời gian giữ thêm sau khi ứng dụng đã mở (ms)
    $description: Enough to see the bounces. Only with the option above on (0-5000, default 1500)
    $description:pt-BR: O suficiente para ver os pulos. Só com a opção acima ligada (0-5000, padrão 1500)
    $description:zh-CN: 足够看到跳动。仅在上面的选项开启时有效（0-5000，默认 1500）
    $description:de-DE: Genug, um das Hüpfen zu sehen. Nur mit eingeschalteter Option oben (0-5000, Standard 1500)
    $description:es: Suficiente para ver los rebotes. Solo con la opción de arriba activada (0-5000, predeterminado 1500)
    $description:ru-RU: Достаточно, чтобы увидеть подпрыгивания. Только при включённом параметре выше (0-5000, по умолчанию 1500)
    $description:ja-JP: バウンスが見える程度の長さです。上のオプションがオンのときのみ（0-5000、既定値 1500）
    $description:vi-VN: Đủ lâu để bạn nhìn thấy trọn vẹn nhịp nảy của icon. Chỉ có tác dụng khi bật tùy chọn ngay phía trên (0-5000, mặc định 1500)
  $name: App icons (dock and transparent taskbar)
  $name:pt-BR: Ícones dos apps (dock e barra transparente)
  $name:zh-CN: 应用图标（程序坞和透明任务栏）
  $name:de-DE: App-Symbole (Dock und transparente Taskleiste)
  $name:es: Iconos de apps (dock y barra de tareas transparente)
  $name:ru-RU: Значки приложений (док и прозрачная панель задач)
  $name:ja-JP: アプリのアイコン（Dock と透明タスクバー）
  $name:vi-VN: Biểu tượng ứng dụng (Dock & Taskbar trong suốt)
  $description: Only for the "Dock" and "Windows taskbar, transparent" choices above (and "only the apps" with its option below on)
  $description:pt-BR: Só para as opções "Dock" e "Barra do Windows transparente" acima (e "só com os apps" com a opção dela abaixo ligada)
  $description:zh-CN: 仅适用于上面的"程序坞"和"Windows 任务栏，透明"选项（开启下面的选项时也适用于"只显示应用"）
  $description:de-DE: Nur für die Auswahl "Dock" und "Windows-Taskleiste, transparent" oben (und "nur die Apps" mit ihrer Option unten eingeschaltet)
  $description:es: Solo para las opciones "Dock" y "Barra de tareas de Windows, transparente" de arriba (y "solo las apps" con su opción de abajo activada)
  $description:ru-RU: Только для вариантов «Док» и «Панель задач Windows, прозрачная» выше (и «только приложения» при включённом параметре ниже)
  $description:ja-JP: 上の「Dock」と「Windows タスクバー、透明」の選択肢のみ（下のオプションがオンなら「アプリのみ」も）
  $description:vi-VN: Chỉ dành cho các lựa chọn "Dock" và "Taskbar Windows trong suốt" ở trên (và "chỉ hiện ứng dụng" khi bật tùy chọn của nó bên dưới)
- dock:
  - iconSize: 32
    $name: Icon size (px)
    $name:pt-BR: Tamanho dos ícones (px)
    $name:zh-CN: 图标大小（像素）
    $name:de-DE: Symbolgröße (px)
    $name:es: Tamaño de iconos (px)
    $name:ru-RU: Размер значков (px)
    $name:ja-JP: アイコンのサイズ (px)
    $name:vi-VN: Kích thước icon (px)
    $description: At rest, like on macOS; the dock's height and margins follow them. When the taskbar doesn't hide itself, the dock at rest fits in the taskbar's height (only the magnified icons go over the windows), so its icons may be smaller (24-64, default 32)
    $description:pt-BR: Em repouso, como no macOS; a altura e as margens da dock acompanham. Quando a barra não se esconde sozinha, a dock em repouso cabe na altura da barra (só os ícones ampliados passam por cima das janelas), então os ícones podem ficar menores (24-64, padrão 32)
    $description:zh-CN: 静止时的大小，像 macOS 一样；程序坞的高度和边距随之变化。任务栏不自动隐藏时，静止的程序坞会放进任务栏的高度内（只有放大的图标会盖住窗口），所以图标可能更小（24-64，默认 32）
    $description:de-DE: In Ruhe, wie unter macOS; Höhe und Ränder des Docks richten sich danach. Wenn sich die Taskleiste nicht selbst ausblendet, passt das Dock in Ruhe in die Taskleistenhöhe (nur die vergrößerten Symbole ragen über die Fenster), daher können die Symbole kleiner sein (24-64, Standard 32)
    $description:es: En reposo, como en macOS; la altura y los márgenes del dock los siguen. Si la barra de tareas no se oculta sola, el dock en reposo cabe en la altura de la barra (solo los iconos ampliados pasan por encima de las ventanas), por lo que sus iconos pueden ser más pequeños (24-64, predeterminado 32)
    $description:ru-RU: В покое, как в macOS; высота и поля дока следуют за ними. Если панель задач не скрывается сама, док в покое вписывается в высоту панели задач (поверх окон выходят только увеличенные значки), поэтому значки могут быть меньше (24-64, по умолчанию 32)
    $description:ja-JP: 通常時のサイズ（macOS と同様）。Dock の高さと余白もそれに合わせて変わります。タスクバーが自動で隠れない場合、通常時の Dock はタスクバーの高さに収まる（拡大したアイコンだけがウィンドウの上に出る）ため、アイコンが小さくなることがあります（24-64、既定値 32）
    $description:vi-VN: Kích thước icon ở trạng thái bình thường (chiều cao và lề của Dock sẽ tự co giãn theo). Nếu bạn không bật tự động ẩn taskbar, thanh Dock lúc nghỉ sẽ tự thu vừa trong chiều cao chuẩn của taskbar Windows nên icon có thể nhỏ hơn mức bạn đặt (24-64, mặc định 32)
  - iconGap: 8
    $name: Space between icons (px)
    $name:pt-BR: Espaço entre os ícones (px)
    $name:zh-CN: 图标间距（像素）
    $name:de-DE: Abstand zwischen Symbolen (px)
    $name:es: Espacio entre iconos (px)
    $name:ru-RU: Расстояние между значками (px)
    $name:ja-JP: アイコンの間隔 (px)
    $name:vi-VN: Khoảng cách giữa các icon (px)
    $description: The dock gets wider with it (0-32, default 8)
    $description:pt-BR: A dock fica mais larga junto (0-32, padrão 8)
    $description:zh-CN: 程序坞会随之变宽（0-32，默认 8）
    $description:de-DE: Das Dock wird dadurch breiter (0-32, Standard 8)
    $description:es: El dock se hace más ancho con él (0-32, predeterminado 8)
    $description:ru-RU: Док становится шире вместе с ним (0-32, по умолчанию 8)
    $description:ja-JP: これに合わせて Dock も広くなります（0-32、既定値 8）
    $description:vi-VN: Tăng khoảng cách này sẽ làm thanh Dock dài ra tương ứng (0-32, mặc định 8)
  - bottomGap: 2
    $name: Gap at the bottom (px)
    $name:pt-BR: Espaço embaixo (px)
    $name:zh-CN: 底部间距（像素）
    $name:de-DE: Abstand unten (px)
    $name:es: Espacio inferior (px)
    $name:ru-RU: Отступ снизу (px)
    $name:ja-JP: 下端の余白 (px)
    $name:vi-VN: Khoảng cách nổi so với mép dưới (px)
    $description: Between the dock and the bottom edge of the screen (0-24, default 2)
    $description:pt-BR: Entre a dock e a borda de baixo da tela (0-24, padrão 2)
    $description:zh-CN: 程序坞与屏幕底部边缘之间的距离（0-24，默认 2）
    $description:de-DE: Zwischen dem Dock und dem unteren Bildschirmrand (0-24, Standard 2)
    $description:es: Entre el dock y el borde inferior de la pantalla (0-24, predeterminado 2)
    $description:ru-RU: Между доком и нижним краем экрана (0-24, по умолчанию 2)
    $description:ja-JP: Dock と画面の下端の間（0-24、既定値 2）
    $description:vi-VN: Khoảng hở giữa đáy thanh Dock và cạnh dưới cùng của màn hình (0-24, mặc định 2)
  - cornerRadius: 22
    $name: Corner roundness (px)
    $name:pt-BR: Arredondamento dos cantos (px)
    $name:zh-CN: 圆角大小（像素）
    $name:de-DE: Eckenrundung (px)
    $name:es: Redondez de las esquinas (px)
    $name:ru-RU: Скругление углов (px)
    $name:ja-JP: 角の丸み (px)
    $name:vi-VN: Độ bo tròn góc Dock (px)
    $description: 0 is square, 32 is very round (0-32, default 22)
    $description:pt-BR: 0 é quadrado, 32 é bem arredondado (0-32, padrão 22)
    $description:zh-CN: 0 为直角，32 为很圆（0-32，默认 22）
    $description:de-DE: 0 ist eckig, 32 sehr rund (0-32, Standard 22)
    $description:es: 0 es cuadrado, 32 muy redondo (0-32, predeterminado 22)
    $description:ru-RU: 0 — прямые, 32 — очень круглые (0-32, по умолчанию 22)
    $description:ja-JP: 0 は直角、32 は非常に丸い（0-32、既定値 22）
    $description:vi-VN: Đặt 0 là góc vuông, 32 là bo tròn tối đa (0-32, mặc định 22)
  - maxWidth: 0
    $name: Widest the dock gets (% of the screen)
    $name:pt-BR: Largura máxima da dock (% da tela)
    $name:zh-CN: 程序坞的最大宽度（屏幕的 %）
    $name:de-DE: Größte Breite des Docks (% des Bildschirms)
    $name:es: Ancho máximo del dock (% de la pantalla)
    $name:ru-RU: Наибольшая ширина дока (% экрана)
    $name:ja-JP: Dock の最大幅（画面の %）
    $name:vi-VN: Độ rộng tối đa của Dock (% màn hình)
    $description: With more apps than fit, the dock stays this wide, like a box; the apps past its ends are cut off and the mouse wheel over it scrolls them. 0 is no limit (0 or 30-100, default 0)
    $description:pt-BR: Com mais apps do que cabem, a dock fica com essa largura, como uma caixa; os apps além das pontas ficam cortados e a roda do mouse sobre ela os rola. 0 é sem limite (0 ou 30-100, padrão 0)
    $description:zh-CN: 应用多到放不下时，程序坞保持这个宽度，像一个收纳盒；超出两端的应用被裁掉，在其上滚动鼠标滚轮即可滚动它们。0 为不限制（0 或 30-100，默认 0）
    $description:de-DE: Bei mehr Apps, als hineinpassen, bleibt das Dock so breit wie eine Box; die Apps hinter seinen Enden werden abgeschnitten und das Mausrad darüber scrollt sie. 0 ist ohne Grenze (0 oder 30-100, Standard 0)
    $description:es: Con más apps de las que caben, el dock se queda así de ancho, como una caja; las apps más allá de sus extremos se cortan y la rueda del ratón sobre él las desplaza. 0 es sin límite (0 o 30-100, predeterminado 0)
    $description:ru-RU: Если приложений больше, чем помещается, док остаётся такой ширины, как коробка; приложения за его краями обрезаются, а колесо мыши над ним их прокручивает. 0 — без ограничения (0 или 30-100, по умолчанию 0)
    $description:ja-JP: アプリが収まりきらないとき、Dock はこの幅のまま箱のようになり、端からはみ出たアプリは切り取られ、上でマウス ホイールを回すとスクロールします。0 は制限なし（0 または 30-100、既定値 0）
    $description:vi-VN: Khi có nhiều ứng dụng hơn chỗ chứa, Dock giữ nguyên độ rộng này như một cái hộp; các ứng dụng vượt quá hai đầu bị cắt và lăn chuột trên Dock sẽ cuộn chúng. 0 là không giới hạn (0 hoặc 30-100, mặc định 0)
  - background: true
    $name: Background behind the icons
    $name:pt-BR: Fundo atrás dos ícones
    $name:zh-CN: 图标后面的背景
    $name:de-DE: Hintergrund hinter den Symbolen
    $name:es: Fondo detrás de los iconos
    $name:ru-RU: Фон за значками
    $name:ja-JP: アイコンの背景
    $name:vi-VN: Hiển thị nền thanh Dock
    $description: The rounded shelf with its thin edge (also around the tray). Off, only the icons show, with the same sizes and magnification
    $description:pt-BR: A base arredondada com a borda fina (também em volta da bandeja). Desligado, só os ícones aparecem, com os mesmos tamanhos e a mesma ampliação
    $description:zh-CN: 带细边框的圆角底座（托盘周围也有）。关闭后只显示图标，大小和放大效果不变
    $description:de-DE: Das abgerundete Regal mit dünnem Rand (auch um den Infobereich). Aus, werden nur die Symbole mit denselben Größen und derselben Vergrößerung angezeigt
    $description:es: El estante redondeado con su borde fino (también alrededor de la bandeja). Desactivado, solo se ven los iconos, con los mismos tamaños y ampliación
    $description:ru-RU: Скруглённая подложка с тонкой рамкой (также вокруг области уведомлений). Если выключено, показываются только значки, с теми же размерами и увеличением
    $description:ja-JP: 細い縁取りのある角丸の台座（トレイの周りにも）。オフにするとアイコンだけが表示され、サイズと拡大は同じです
    $description:vi-VN: Phần khung nền bo tròn có viền mảnh phía sau các icon. Khi tắt, chỉ còn các icon nổi trên màn hình nhưng vẫn giữ nguyên kích thước và hiệu ứng phóng đại
  - backgroundOpacity: 50
    $name: Background color (%)
    $name:pt-BR: Cor do fundo (%)
    $name:zh-CN: 背景颜色（%）
    $name:de-DE: Hintergrundfarbe (%)
    $name:es: Color de fondo (%)
    $name:ru-RU: Цвет фона (%)
    $name:ja-JP: 背景の色 (%)
    $name:vi-VN: Độ đậm nền Dock (%)
    $description: How much color the background has, in the theme's color; 0 is see-through (only its edge shows), 100 is solid (0-100, default 50)
    $description:pt-BR: Quanto de cor o fundo tem, na cor do tema; 0 é transparente (só a borda aparece), 100 é sólido (0-100, padrão 50)
    $description:zh-CN: 背景的颜色有多浓，使用主题颜色；0 为透明（只显示边框），100 为纯色（0-100，默认 50）
    $description:de-DE: Wie viel Farbe der Hintergrund hat, in der Farbe des Designs; 0 ist durchsichtig (nur der Rand ist sichtbar), 100 deckend (0-100, Standard 50)
    $description:es: Cuánto color tiene el fondo, en el color del tema; 0 es transparente (solo se ve su borde), 100 sólido (0-100, predeterminado 50)
    $description:ru-RU: Насколько фон окрашен цветом темы; 0 — прозрачный (виден только край), 100 — сплошной (0-100, по умолчанию 50)
    $description:ja-JP: 背景にテーマの色をどれだけ付けるか。0 は透明（縁のみ表示）、100 は不透明（0-100、既定値 50）
    $description:vi-VN: Độ đậm nhạt của lớp màu nền thanh Dock; 0 là trong suốt hoàn toàn (chỉ thấy viền mảnh), 100 là đặc hoàn toàn (0-100, mặc định 50)
  - dotIndicator: true
    $name: Dots under open apps
    $name:pt-BR: Pontinhos embaixo dos apps abertos
    $name:zh-CN: 已打开应用下方的圆点
    $name:de-DE: Punkte unter geöffneten Apps
    $name:es: Puntos bajo las apps abiertas
    $name:ru-RU: Точки под открытыми приложениями
    $name:ja-JP: 開いているアプリの下のドット
    $name:vi-VN: Hiện dấu chấm tròn dưới ứng dụng đang mở
    $description: Instead of the Windows line
    $description:pt-BR: No lugar do traço do Windows
    $description:zh-CN: 代替 Windows 的横线
    $description:de-DE: Anstelle der Windows-Linie
    $description:es: En lugar de la línea de Windows
    $description:ru-RU: Вместо линии Windows
    $description:ja-JP: Windows の線の代わりに表示します
    $description:vi-VN: Thay thế cho vạch ngang mặc định của Windows 11
  - attentionMode: icon
    $name: When an app needs attention with the dock hidden
    $name:pt-BR: Quando um app pede atenção com a dock escondida
    $name:zh-CN: 程序坞隐藏时应用需要注意
    $name:de-DE: Wenn eine App Aufmerksamkeit braucht und das Dock ausgeblendet ist
    $name:es: Cuando una app necesita atención con el dock oculto
    $name:ru-RU: Когда приложение требует внимания при скрытом доке
    $name:ja-JP: Dock が非表示のときにアプリが注意を求めたら
    $name:vi-VN: Khi có ứng dụng báo hiệu lúc Dock đang ẩn
    $description: An app flashing its taskbar button (a message, a download done) while the dock is hidden
    $description:pt-BR: Um app piscando o botão na barra (uma mensagem, um download pronto) enquanto a dock está escondida
    $description:zh-CN: 程序坞隐藏时，某个应用闪烁它的任务栏按钮（一条消息、一个下载完成）
    $description:de-DE: Eine App, deren Taskleisten-Schaltfläche blinkt (eine Nachricht, ein fertiger Download), während das Dock ausgeblendet ist
    $description:es: Una app cuyo botón de la barra de tareas parpadea (un mensaje, una descarga terminada) mientras el dock está oculto
    $description:ru-RU: Приложение мигает кнопкой на панели задач (сообщение, завершённая загрузка), пока док скрыт
    $description:ja-JP: Dock が非表示の間に、アプリがタスクバーのボタンを点滅させたとき（メッセージ、ダウンロード完了など）
    $description:vi-VN: Cách báo cho bạn biết khi một ứng dụng nhấp nháy icon trên taskbar (có tin nhắn mới, tải tệp xong...) trong lúc thanh Dock đang ẩn
    $options:
    - icon: Only the app's icon peeks out, bouncing
    - dock: The whole dock shows up
    - none: Nothing
    $options:pt-BR:
    - icon: Só o ícone do app aparece, pulando
    - dock: A dock inteira aparece
    - none: Nada
    $options:zh-CN:
    - icon: 只有应用图标探出并跳动
    - dock: 整个程序坞显示出来
    - none: 什么都不做
    $options:de-DE:
    - icon: Nur das App-Symbol schaut hüpfend hervor
    - dock: Das ganze Dock erscheint
    - none: Nichts
    $options:es:
    - icon: Solo asoma el icono de la app, rebotando
    - dock: Aparece todo el dock
    - none: Nada
    $options:ru-RU:
    - icon: Выглядывает только значок приложения, подпрыгивая
    - dock: Появляется весь док
    - none: Ничего
    $options:ja-JP:
    - icon: アプリのアイコンだけがバウンスしながらのぞく
    - dock: Dock 全体が表示される
    - none: 何もしない
    $options:vi-VN:
    - icon: Chỉ nhô riêng icon của ứng dụng đó lên và nảy nhẹ
    - dock: Trượt cả thanh Dock lên
    - none: Không làm gì
  - hideTray: false
    $name: Hide the clock and the tray from the dock (without the island)
    $name:pt-BR: Esconder o relógio e a bandeja da dock (sem a ilha)
    $name:zh-CN: 从程序坞隐藏时钟和托盘（没有灵动岛时）
    $name:de-DE: Uhr und Infobereich im Dock ausblenden (ohne die Insel)
    $name:es: Ocultar el reloj y la bandeja del dock (sin la isla)
    $name:ru-RU: Скрыть часы и область уведомлений из дока (без острова)
    $name:ja-JP: Dock の時計とトレイを非表示（アイランドなし）
    $name:vi-VN: Ẩn đồng hồ và khay hệ thống khỏi thanh Dock (khi tắt đảo)
    $description: Only for the "Dock" choice with the island at the top off. The dock shows only the apps. With the island on, the dock never shows them (the island does), so this changes nothing then
    $description:pt-BR: Só para a opção "Dock" com a ilha no topo desligada. A dock mostra só os apps. Com a ilha ligada, a dock nunca mostra os dois (a ilha mostra), então esta opção não muda nada
    $description:zh-CN: 仅适用于"程序坞"选项，并且顶部灵动岛关闭时：程序坞只显示应用。灵动岛开启时，程序坞从不显示它们（由灵动岛显示），所以这时此选项不起作用
    $description:de-DE: Nur für die Auswahl "Dock" bei ausgeschalteter Insel oben. Das Dock zeigt nur die Apps. Bei eingeschalteter Insel zeigt das Dock sie nie (die Insel tut es), daher ändert dies dann nichts
    $description:es: Solo para la opción "Dock" con la isla arriba desactivada. El dock muestra solo las apps. Con la isla activada, el dock nunca los muestra (lo hace la isla), así que esto no cambia nada entonces
    $description:ru-RU: Только для варианта «Док» при выключенном острове вверху. Док показывает только приложения. При включённом острове док никогда их не показывает (это делает остров), поэтому тогда параметр ничего не меняет
    $description:ja-JP: 上部のアイランドがオフのときの「Dock」専用です。Dock にはアプリだけが表示されます。アイランドがオンのときは Dock には表示されず（アイランドが表示します）、このオプションは何も変更しません
    $description:vi-VN: Chỉ dùng khi bạn chọn kiểu "Dock" nhưng tắt thanh đảo ở cạnh trên, giúp thanh Dock chỉ hiện thuần các icon ứng dụng. Còn khi bật đảo, đồng hồ và khay hệ thống đã tự chuyển lên đảo nên tùy chọn này không cần thiết
  $name: Dock look (dock only)
  $name:pt-BR: Visual da dock (só para a dock)
  $name:zh-CN: 程序坞外观（仅程序坞）
  $name:de-DE: Dock-Aussehen (nur Dock)
  $name:es: Aspecto del dock (solo dock)
  $name:ru-RU: Вид дока (только для дока)
  $name:ja-JP: Dock の外観（Dock のみ）
  $name:vi-VN: Giao diện thanh Dock (chỉ dành cho Dock)
  $description: Only for the "Dock" choice above
  $description:pt-BR: Só para a opção "Dock" acima
  $description:zh-CN: 仅适用于上面的"程序坞"选项
  $description:de-DE: Nur für die Auswahl "Dock" oben
  $description:es: Solo para la opción "Dock" de arriba
  $description:ru-RU: Только для варианта «Док» выше
  $description:ja-JP: 上の「Dock」の選択肢のみ
  $description:vi-VN: Chỉ áp dụng khi chọn chế độ "Dock" ở mục đầu tiên
- animations:
  - enabled: true
    $name: Animations
    $name:pt-BR: Animações
    $name:zh-CN: 动画
    $name:de-DE: Animationen
    $name:es: Animaciones
    $name:ru-RU: Анимации
    $name:ja-JP: アニメーション
    $name:vi-VN: Bật hiệu ứng chuyển động
    $description: The dock's and the transparent taskbar's animations (sliding, magnifying, bouncing). Off, everything changes at once
    $description:pt-BR: As animações da dock e da barra transparente (deslizar, ampliar, pular). Desligadas, tudo muda na hora
    $description:zh-CN: 程序坞和透明任务栏的动画（滑动、放大、弹跳）。关闭后，一切立即变化
    $description:de-DE: Die Animationen des Docks und der transparenten Taskleiste (Gleiten, Vergrößern, Hüpfen). Aus, ändert sich alles auf einmal
    $description:es: Las animaciones del dock y de la barra de tareas transparente (deslizar, ampliar, rebotar). Desactivadas, todo cambia de golpe
    $description:ru-RU: Анимации дока и прозрачной панели задач (выезд, увеличение, подпрыгивание). Если выключено, всё меняется мгновенно
    $description:ja-JP: Dock と透明タスクバーのアニメーション（スライド、拡大、バウンス）。オフにすると、すべてが即座に切り替わります
    $description:vi-VN: Hiệu ứng trượt, phóng đại sóng và nảy icon của thanh Dock / taskbar trong suốt. Khi tắt, mọi thao tác sẽ phản hồi tức thì
  - level: all
    $name: How many
    $name:pt-BR: Quantas
    $name:zh-CN: 数量
    $name:de-DE: Wie viele
    $name:es: Cuántas
    $name:ru-RU: Сколько
    $name:ja-JP: 量
    $name:vi-VN: Mức độ hiệu ứng
    $description: '"Partial" keeps the taskbar sliding in and out, and drops what only decorates, for slower computers'
    $description:pt-BR: '"Parciais" mantém a barra entrando e saindo, e tira o que só enfeita, para computadores mais lentos'
    $description:zh-CN: '"部分"保留任务栏滑入和滑出的动画，去掉只用于装饰的动画，适合较慢的电脑'
    $description:de-DE: '"Teilweise" lässt die Taskleiste weiter ein- und ausgleiten und lässt weg, was nur dekoriert, für langsamere Computer'
    $description:es: '"Parcial" mantiene el deslizamiento de la barra de tareas y quita lo que solo decora, para equipos más lentos'
    $description:ru-RU: '«Частично» оставляет выезд и уход панели задач и убирает то, что только украшает, для медленных компьютеров'
    $description:ja-JP: '「一部」はタスクバーのスライドイン/アウトを残し、装飾だけのものを省きます。低速な PC 向けです'
    $description:vi-VN: 'Chọn "Cơ bản" để giữ lại hiệu ứng trượt ẩn/hiện chính và tắt bớt các hiệu ứng trang trí phụ, giúp nhẹ máy hơn'
    $options:
    - all: All
    - partial: "Partial: no bounces, glows, color blends, counting numbers nor scrolling text"
    - none: None
    $options:pt-BR:
    - all: Todas
    - partial: "Parciais: sem quiques, brilhos, mistura de cores, números contando nem texto rolando"
    - none: Nenhuma
    $options:zh-CN:
    - all: 全部
    - partial: "部分：没有弹跳、发光、颜色渐变、数字滚动和文字滚动"
    - none: 无
    $options:de-DE:
    - all: Alle
    - partial: "Teilweise: kein Hüpfen, Leuchten, Farbübergänge, Zählzahlen oder Lauftext"
    - none: Keine
    $options:es:
    - all: Todas
    - partial: "Parcial: sin rebotes, brillos, mezclas de color, números que cuentan ni texto que se desplaza"
    - none: Ninguna
    $options:ru-RU:
    - all: Все
    - partial: "Частично: без подпрыгиваний, свечения, смешения цветов, счёта чисел и прокрутки текста"
    - none: Нет
    $options:ja-JP:
    - all: すべて
    - partial: "一部: バウンス、グロー、色のブレンド、数字のカウント、テキストのスクロールなし"
    - none: なし
    $options:vi-VN:
    - all: Đầy đủ
    - partial: "Cơ bản: tắt hiệu ứng nảy, phát sáng, chuyển màu, nhảy số và chạy chữ"
    - none: Tắt hết
  - speed: 125
    $name: Speed (%)
    $name:pt-BR: Velocidade (%)
    $name:zh-CN: 速度（%）
    $name:de-DE: Geschwindigkeit (%)
    $name:es: Velocidad (%)
    $name:ru-RU: Скорость (%)
    $name:ja-JP: 速度 (%)
    $name:vi-VN: Tốc độ hiệu ứng (%)
    $description: More is faster, less is slower (50-200, default 125)
    $description:pt-BR: Mais é mais rápido, menos é mais lento (50-200, padrão 125)
    $description:zh-CN: 越大越快，越小越慢（50-200，默认 125）
    $description:de-DE: Mehr ist schneller, weniger langsamer (50-200, Standard 125)
    $description:es: Más es más rápido, menos más lento (50-200, predeterminado 125)
    $description:ru-RU: Больше — быстрее, меньше — медленнее (50-200, по умолчанию 125)
    $description:ja-JP: 大きいほど速く、小さいほど遅くなります（50-200、既定値 125）
    $description:vi-VN: Số càng lớn chuyển động càng nhanh, số càng nhỏ càng chậm lại (50-200, mặc định 125)
  - frameRate: screen
    $name: Frame rate limit
    $name:pt-BR: Limite de quadros por segundo
    $name:zh-CN: 帧率限制
    $name:de-DE: Bildratenlimit
    $name:es: Límite de fotogramas
    $name:ru-RU: Ограничение частоты кадров
    $name:ja-JP: フレーム レートの上限
    $name:vi-VN: Giới hạn tốc độ khung hình (FPS)
    $description: Fewer frames cost less on slower computers; the animations keep their length
    $description:pt-BR: Menos quadros custam menos em computadores mais lentos; as animações mantêm a duração
    $description:zh-CN: 较少的帧在较慢的电脑上开销更小；动画时长不变
    $description:de-DE: Weniger Bilder kosten auf langsameren Computern weniger; die Animationen behalten ihre Dauer
    $description:es: Menos fotogramas cuestan menos en equipos más lentos; las animaciones mantienen su duración
    $description:ru-RU: Меньше кадров — меньше нагрузка на медленных компьютерах; длительность анимаций сохраняется
    $description:ja-JP: フレーム数が少ないほど低速な PC での負荷が下がります。アニメーションの長さは変わりません
    $description:vi-VN: Giảm số khung hình giúp tiết kiệm tài nguyên hơn trên máy cấu hình yếu mà không làm thay đổi tốc độ của hiệu ứng
    $options:
    - screen: The screen's rate
    - fps90: About 90 a second
    - fps60: About 60 a second
    - fps30: About 30 a second
    $options:pt-BR:
    - screen: A taxa da tela
    - fps90: Cerca de 90 por segundo
    - fps60: Cerca de 60 por segundo
    - fps30: Cerca de 30 por segundo
    $options:zh-CN:
    - screen: 屏幕刷新率
    - fps90: 约每秒 90 帧
    - fps60: 约每秒 60 帧
    - fps30: 约每秒 30 帧
    $options:de-DE:
    - screen: Die Rate des Bildschirms
    - fps90: Etwa 90 pro Sekunde
    - fps60: Etwa 60 pro Sekunde
    - fps30: Etwa 30 pro Sekunde
    $options:es:
    - screen: La tasa de la pantalla
    - fps90: Unos 90 por segundo
    - fps60: Unos 60 por segundo
    - fps30: Unos 30 por segundo
    $options:ru-RU:
    - screen: Частота экрана
    - fps90: Около 90 в секунду
    - fps60: Около 60 в секунду
    - fps30: Около 30 в секунду
    $options:ja-JP:
    - screen: 画面のリフレッシュ レート
    - fps90: 約 90 フレーム/秒
    - fps60: 約 60 フレーム/秒
    - fps30: 約 30 フレーム/秒
    $options:vi-VN:
    - screen: Theo tần số quét màn hình
    - fps90: Khoảng 90 FPS
    - fps60: Khoảng 60 FPS
    - fps30: Khoảng 30 FPS
  $name: Dock animations
  $name:pt-BR: Animações da dock
  $name:zh-CN: 程序坞动画
  $name:de-DE: Dock-Animationen
  $name:es: Animaciones del dock
  $name:ru-RU: Анимации дока
  $name:ja-JP: Dock のアニメーション
  $name:vi-VN: Chuyển động của thanh Dock
  $description: For the dock and the transparent taskbar. The island's own animations are in its settings (they start as these were)
  $description:pt-BR: Para a dock e a barra transparente. As animações da ilha ficam nas configurações dela (começam como estas estavam)
  $description:zh-CN: 适用于程序坞和透明任务栏。灵动岛自己的动画在它的设置中（初始与这里相同）
  $description:de-DE: Für das Dock und die transparente Taskleiste. Die eigenen Animationen der Insel stehen in ihren Einstellungen (sie starten wie diese)
  $description:es: Para el dock y la barra de tareas transparente. Las animaciones propias de la isla están en su configuración (empiezan como estas)
  $description:ru-RU: Для дока и прозрачной панели задач. Анимации острова — в его настройках (они начинаются такими же)
  $description:ja-JP: Dock と透明タスクバー用です。アイランドのアニメーションはその設定にあります（初期値はこれらと同じ）
  $description:vi-VN: Áp dụng cho thanh Dock và taskbar trong suốt. Riêng hiệu ứng chuyển động của đảo được chỉnh trực tiếp trong cửa sổ Cài đặt của đảo
*/
// ==/WindhawkModSettings==
