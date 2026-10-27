#!/usr/bin/env python3
"""Pacote de idiomas do Nexus Modeler.

Cada linha da tabela é uma frase da tela em português (a chave) seguida das
traduções, na ordem de IDIOMAS. Rodar este arquivo gera assets/idiomas/
idioma_<código>.txt no formato de tradução do JUCE, que o plugin embute e o
instalador copia. Para corrigir uma tradução, mude aqui e rode de novo.

Uso: python3 tools/idiomas/traducoes.py
"""

from pathlib import Path

# código, nome na própria língua, países (cabeçalho do JUCE)
IDIOMAS = [
    ("en", "English", "us gb au ca"),
    ("es", "Español", "es mx ar co cl"),
    ("fr", "Français", "fr ca be ch"),
    ("it", "Italiano", "it ch"),
    ("de", "Deutsch", "de at ch"),
    ("ja", "日本語", "jp"),
    ("zh", "中文（简体）", "cn sg"),
    ("ko", "한국어", "kr"),
    ("hi", "हिन्दी", "in"),
    ("ar", "العربية", "sa eg ae ma"),
    ("bn", "বাংলা", "bd in"),
    ("ru", "Русский", "ru"),
    ("id", "Bahasa Indonesia", "id"),
    ("tr", "Türkçe", "tr"),
]

# chave em português | en | es | fr | it | de | ja | zh | ko | hi | ar | bn | ru | id | tr
TABELA = [
    # Rótulos dos painéis (caixas de texto por cima dos equipamentos 3D)
    ("GANHO", "GAIN", "GANANCIA", "GAIN", "GAIN", "GAIN", "ゲイン", "增益", "게인", "गेन", "الكسب", "গেইন", "ГЕЙН", "GAIN", "GAIN"),
    ("GRAVE", "BASS", "GRAVES", "GRAVES", "BASSI", "BASS", "ベース", "低音", "베이스", "बास", "الجهير", "বেস", "НИЗ", "BASS", "BAS"),
    ("MÉDIO", "MIDDLE", "MEDIOS", "MÉDIUMS", "MEDI", "MITTEN", "ミドル", "中音", "미들", "मिड", "الوسط", "মিড", "СЕРЕДИНА", "MIDDLE", "ORTA"),
    ("AGUDO", "TREBLE", "AGUDOS", "AIGUS", "ALTI", "HÖHEN", "トレブル", "高音", "트레블", "ट्रेबल", "الحدة", "ট্রেবল", "ВЕРХ", "TREBLE", "TİZ"),
    ("PRESENÇA", "PRESENCE", "PRESENCIA", "PRÉSENCE", "PRESENCE", "PRESENCE", "プレゼンス", "临场感", "프레즌스", "प्रेज़ेंस", "الحضور", "প্রেজেন্স", "ПРЕЗЕНС", "PRESENCE", "PRESENCE"),
    ("VOLUME", "VOLUME", "VOLUMEN", "VOLUME", "VOLUME", "VOLUME", "ボリューム", "音量", "볼륨", "वॉल्यूम", "الصوت", "ভলিউম", "ГРОМКОСТЬ", "VOLUME", "SES"),
    ("LIGA", "POWER", "ENCENDIDO", "MARCHE", "ACCESO", "NETZ", "電源", "电源", "전원", "पावर", "تشغيل", "পাওয়ার", "ПИТАНИЕ", "DAYA", "GÜÇ"),
    ("ALTO", "HIGH", "ALTO", "HAUT", "ALTO", "HIGH", "ハイ", "高", "하이", "हाई", "عالٍ", "হাই", "ВЫС.", "TINGGI", "YÜKSEK"),
    ("BAIXO", "LOW", "BAJO", "BAS", "BASSO", "LOW", "ロー", "低", "로우", "लो", "منخفض", "লো", "НИЗК.", "RENDAH", "DÜŞÜK"),
    ("TOM", "TONE", "TONO", "TONALITÉ", "TONO", "TON", "トーン", "音色", "톤", "टोन", "النغمة", "টোন", "ТОН", "NADA", "TON"),
    ("NÍVEL", "LEVEL", "NIVEL", "NIVEAU", "LIVELLO", "PEGEL", "レベル", "电平", "레벨", "लेवल", "المستوى", "লেভেল", "УРОВЕНЬ", "LEVEL", "SEVİYE"),
    ("DISTORÇÃO", "DISTORTION", "DISTORSIÓN", "DISTORSION", "DISTORSIONE", "VERZERRUNG", "ディストーション", "失真", "디스토션", "डिस्टॉर्शन", "التشويه", "ডিস্টরশন", "ИСКАЖЕНИЕ", "DISTORSI", "DİSTORSİYON"),
    ("SAÍDA", "OUTPUT", "SALIDA", "SORTIE", "USCITA", "AUSGANG", "出力", "输出", "출력", "आउटपुट", "الخرج", "আউটপুট", "ВЫХОД", "KELUARAN", "ÇIKIŞ"),
    # Canais do cabeçote
    ("Normal", "Normal", "Normal", "Normal", "Normale", "Normal", "ノーマル", "标准", "노멀", "नॉर्मल", "عادي", "নরমাল", "Нормальный", "Normal", "Normal"),
    ("Limpo", "Clean", "Limpio", "Clair", "Pulito", "Clean", "クリーン", "清音", "클린", "क्लीन", "نظيف", "ক্লিন", "Чистый", "Clean", "Temiz"),
    ("Crunch", "Crunch", "Crunch", "Crunch", "Crunch", "Crunch", "クランチ", "过载", "크런치", "क्रंच", "كرانش", "ক্রাঞ্চ", "Кранч", "Crunch", "Crunch"),
    ("Alto ganho", "High gain", "Alta ganancia", "Haut gain", "Alto guadagno", "High Gain", "ハイゲイン", "高增益", "하이 게인", "हाई गेन", "كسب عالٍ", "হাই গেইন", "Высокий гейн", "High gain", "Yüksek gain"),
    # Botões (balão ao girar)
    ("Ganho", "Gain", "Ganancia", "Gain", "Gain", "Gain", "ゲイン", "增益", "게인", "गेन", "الكسب", "গেইন", "Гейн", "Gain", "Gain"),
    ("Grave", "Bass", "Graves", "Graves", "Bassi", "Bass", "ベース", "低音", "베이스", "बास", "الجهير", "বেস", "Низ", "Bass", "Bas"),
    ("Médio", "Middle", "Medios", "Médiums", "Medi", "Mitten", "ミドル", "中音", "미들", "मिड", "الوسط", "মিড", "Середина", "Middle", "Orta"),
    ("Agudo", "Treble", "Agudos", "Aigus", "Alti", "Höhen", "トレブル", "高音", "트레블", "ट्रेबल", "الحدة", "ট্রেবল", "Верх", "Treble", "Tiz"),
    ("Presença", "Presence", "Presencia", "Présence", "Presence", "Presence", "プレゼンス", "临场感", "프레즌스", "प्रेज़ेंस", "الحضور", "প্রেজেন্স", "Презенс", "Presence", "Presence"),
    ("Volume", "Volume", "Volumen", "Volume", "Volume", "Volume", "ボリューム", "音量", "볼륨", "वॉल्यूम", "الصوت", "ভলিউম", "Громкость", "Volume", "Ses"),
    ("Tom", "Tone", "Tono", "Tonalité", "Tono", "Ton", "トーン", "音色", "톤", "टोन", "النغمة", "টোন", "Тон", "Nada", "Ton"),
    ("Nível", "Level", "Nivel", "Niveau", "Livello", "Pegel", "レベル", "电平", "레벨", "लेवल", "المستوى", "লেভেল", "Уровень", "Level", "Seviye"),
    ("Entrada", "Input", "Entrada", "Entrée", "Ingresso", "Eingang", "入力", "输入", "입력", "इनपुट", "الدخل", "ইনপুট", "Вход", "Masukan", "Giriş"),
    ("Saída", "Output", "Salida", "Sortie", "Uscita", "Ausgang", "出力", "输出", "출력", "आउटपुट", "الخرج", "আউটপুট", "Выход", "Keluaran", "Çıkış"),
    # Caixa
    ("Desligada", "Off", "Apagada", "Désactivée", "Spenta", "Aus", "オフ", "关闭", "끄기", "बंद", "متوقفة", "বন্ধ", "Выкл.", "Mati", "Kapalı"),
    ("Captura", "Capture", "Captura", "Capture", "Cattura", "Capture", "キャプチャー", "采样", "캡처", "कैप्चर", "الالتقاط", "ক্যাপচার", "Захват", "Tangkapan", "Yakalama"),
    ("IR próprio", "Custom IR", "IR propio", "IR personnalisée", "IR personale", "Eigene IR", "カスタムIR", "自定义 IR", "사용자 IR", "अपना IR", "IR مخصصة", "নিজস্ব IR", "Свой IR", "IR sendiri", "Özel IR"),
    # Caminho do sinal
    ("Pedal", "Pedal", "Pedal", "Pédale", "Pedale", "Pedal", "ペダル", "效果器", "페달", "पेडल", "دوّاسة", "প্যাডেল", "Педаль", "Pedal", "Pedal"),
    ("Amplificador", "Amplifier", "Amplificador", "Ampli", "Amplificatore", "Verstärker", "アンプ", "音箱头", "앰프", "एम्पलीफायर", "المضخم", "অ্যাম্প", "Усилитель", "Amplifier", "Amfi"),
    ("Caixa", "Cabinet", "Caja", "Baffle", "Cassa", "Box", "キャビネット", "箱体", "캐비닛", "कैबिनेट", "الصندوق", "ক্যাবিনেট", "Кабинет", "Kabinet", "Kabin"),
    ("vazio", "empty", "vacío", "vide", "vuoto", "leer", "空", "空", "비어 있음", "खाली", "فارغ", "খালি", "пусто", "kosong", "boş"),
    ("desligado", "off", "apagado", "désactivé", "spento", "aus", "オフ", "关闭", "꺼짐", "बंद", "متوقف", "বন্ধ", "выкл.", "mati", "kapalı"),
    # Janelas de escolha de arquivo
    ("Escolha a pasta do pacote do pedal", "Choose the pedal pack folder", "Elige la carpeta del paquete del pedal", "Choisissez le dossier du pack de la pédale", "Scegli la cartella del pacchetto del pedale", "Ordner des Pedal-Pakets wählen", "ペダルのパックフォルダーを選択", "选择效果器包文件夹", "페달 팩 폴더 선택", "पेडल पैक का फ़ोल्डर चुनें", "اختر مجلد حزمة الدوّاسة", "প্যাডেল প্যাকের ফোল্ডার বেছে নিন", "Выберите папку пакета педали", "Pilih folder paket pedal", "Pedal paketinin klasörünü seçin"),
    ("Escolha a pasta do pacote do amplificador", "Choose the amplifier pack folder", "Elige la carpeta del paquete del amplificador", "Choisissez le dossier du pack de l'ampli", "Scegli la cartella del pacchetto dell'amplificatore", "Ordner des Verstärker-Pakets wählen", "アンプのパックフォルダーを選択", "选择音箱头包文件夹", "앰프 팩 폴더 선택", "एम्पलीफायर पैक का फ़ोल्डर चुनें", "اختر مجلد حزمة المضخم", "অ্যাম্প প্যাকের ফোল্ডার বেছে নিন", "Выберите папку пакета усилителя", "Pilih folder paket amplifier", "Amfi paketinin klasörünü seçin"),
    ("Escolha o IR da caixa", "Choose the cabinet IR", "Elige el IR de la caja", "Choisissez l'IR du baffle", "Scegli l'IR della cassa", "Box-IR wählen", "キャビネットのIRを選択", "选择箱体 IR", "캐비닛 IR 선택", "कैबिनेट का IR चुनें", "اختر IR الصندوق", "ক্যাবিনেটের IR বেছে নিন", "Выберите IR кабинета", "Pilih IR kabinet", "Kabin IR'ını seçin"),
    # Sobre
    ("Plugin de guitarra gratuito e de código aberto (GPL v3).", "Free, open-source guitar plugin (GPL v3).", "Plugin de guitarra gratuito y de código abierto (GPL v3).", "Plugin guitare gratuit et open source (GPL v3).", "Plugin per chitarra gratuito e open source (GPL v3).", "Kostenloses Open-Source-Gitarren-Plugin (GPL v3).", "無料・オープンソースのギタープラグイン (GPL v3)。", "免费开源吉他插件（GPL v3）。", "무료 오픈 소스 기타 플러그인 (GPL v3).", "मुफ़्त और ओपन सोर्स गिटार प्लगइन (GPL v3)।", "إضافة جيتار مجانية ومفتوحة المصدر (GPL v3).", "বিনামূল্যের ওপেন সোর্স গিটার প্লাগইন (GPL v3)।", "Бесплатный гитарный плагин с открытым кодом (GPL v3).", "Plugin gitar gratis dan sumber terbuka (GPL v3).", "Ücretsiz ve açık kaynak gitar eklentisi (GPL v3)."),
    ("Toca capturas NAM A2 e troca de captura conforme o botão gira.", "Plays NAM A2 captures and switches captures as the knob turns.", "Reproduce capturas NAM A2 y cambia de captura al girar la perilla.", "Joue des captures NAM A2 et change de capture quand le bouton tourne.", "Suona catture NAM A2 e cambia cattura mentre la manopola gira.", "Spielt NAM-A2-Captures und wechselt sie beim Drehen des Reglers.", "NAM A2 キャプチャーを再生し、ノブに合わせて切り替えます。", "播放 NAM A2 采样，并随旋钮转动切换采样。", "NAM A2 캡처를 재생하고 노브를 돌리면 캡처를 바꿉니다.", "NAM A2 कैप्चर चलाता है और नॉब घुमाने पर कैप्चर बदलता है।", "يشغّل التقاطات NAM A2 ويبدّلها مع دوران المقبض.", "NAM A2 ক্যাপচার বাজায় এবং নব ঘোরালে ক্যাপচার বদলায়।", "Играет захваты NAM A2 и переключает их при повороте ручки.", "Memutar tangkapan NAM A2 dan berganti tangkapan saat kenop diputar.", "NAM A2 yakalamalarını çalar ve düğme döndükçe yakalamayı değiştirir."),
    # MIDI
    ("Aprender MIDI", "MIDI learn", "Aprender MIDI", "Apprendre MIDI", "Apprendi MIDI", "MIDI lernen", "MIDIラーン", "MIDI 学习", "MIDI 학습", "MIDI सीखें", "تعلّم MIDI", "MIDI শিখুন", "Обучить MIDI", "Pelajari MIDI", "MIDI öğren"),
    ("Trocar controle MIDI (agora CC %1)", "Change MIDI control (now CC %1)", "Cambiar control MIDI (ahora CC %1)", "Changer le contrôle MIDI (actuellement CC %1)", "Cambia controllo MIDI (ora CC %1)", "MIDI-Regler ändern (jetzt CC %1)", "MIDIコントロールを変更（現在 CC %1）", "更换 MIDI 控制（当前 CC %1）", "MIDI 컨트롤 변경 (현재 CC %1)", "MIDI कंट्रोल बदलें (अभी CC %1)", "تغيير تحكم MIDI (الحالي CC %1)", "MIDI কন্ট্রোল বদলান (এখন CC %1)", "Сменить MIDI-контроллер (сейчас CC %1)", "Ganti kontrol MIDI (sekarang CC %1)", "MIDI kontrolünü değiştir (şu an CC %1)"),
    ("Esquecer MIDI (CC %1)", "Forget MIDI (CC %1)", "Olvidar MIDI (CC %1)", "Oublier le MIDI (CC %1)", "Dimentica MIDI (CC %1)", "MIDI vergessen (CC %1)", "MIDIを解除（CC %1）", "取消 MIDI（CC %1）", "MIDI 해제 (CC %1)", "MIDI हटाएँ (CC %1)", "إلغاء MIDI (CC %1)", "MIDI মুছুন (CC %1)", "Забыть MIDI (CC %1)", "Lupakan MIDI (CC %1)", "MIDI'yi unut (CC %1)"),
    ("Cancelar aprender MIDI", "Cancel MIDI learn", "Cancelar aprender MIDI", "Annuler l'apprentissage MIDI", "Annulla apprendimento MIDI", "MIDI-Lernen abbrechen", "MIDIラーンを中止", "取消 MIDI 学习", "MIDI 학습 취소", "MIDI सीखना रद्द करें", "إلغاء تعلّم MIDI", "MIDI শেখা বাতিল", "Отменить обучение MIDI", "Batalkan pelajari MIDI", "MIDI öğrenmeyi iptal et"),
    ("Troca de patch 1, 2, 3 já escolhe o canal", "Program change 1, 2, 3 already selects the channel", "El cambio de programa 1, 2, 3 ya elige el canal", "Le changement de programme 1, 2, 3 choisit déjà le canal", "Il cambio programma 1, 2, 3 sceglie già il canale", "Programmwechsel 1, 2, 3 wählt schon den Kanal", "プログラムチェンジ 1・2・3 でチャンネルを選択", "程序变更 1、2、3 直接选择通道", "프로그램 체인지 1, 2, 3으로 채널 선택", "प्रोग्राम चेंज 1, 2, 3 से चैनल चुना जाता है", "تغيير البرنامج 1 و2 و3 يختار القناة", "প্রোগ্রাম চেঞ্জ 1, 2, 3 চ্যানেল বেছে নেয়", "Смена программы 1, 2, 3 уже выбирает канал", "Program change 1, 2, 3 sudah memilih kanal", "Program değişimi 1, 2, 3 kanalı seçer"),
    ("Mexa no controle da pedaleira para ligar a %1", "Move the controller on your pedalboard to link %1", "Mueve el control de tu pedalera para enlazar %1", "Bougez le contrôle du pédalier pour lier %1", "Muovi il controllo della pedaliera per collegare %1", "Bewege den Regler am Pedalboard, um %1 zu verbinden", "フットコントローラーを動かして %1 に割り当て", "移动脚踏控制器以绑定 %1", "페달보드 컨트롤을 움직여 %1에 연결", "%1 जोड़ने के लिए पेडलबोर्ड का कंट्रोल हिलाएँ", "حرّك أداة التحكم في لوحة الدواسات لربط %1", "%1 যুক্ত করতে প্যাডেলবোর্ডের কন্ট্রোল নাড়ান", "Подвигайте контроллер педалборда, чтобы привязать %1", "Gerakkan kontrol pedalboard untuk menautkan %1", "%1 bağlamak için pedalboard kontrolünü oynatın"),
    ("%1 não responde mais ao MIDI", "%1 no longer responds to MIDI", "%1 ya no responde al MIDI", "%1 ne répond plus au MIDI", "%1 non risponde più al MIDI", "%1 reagiert nicht mehr auf MIDI", "%1 はMIDIに反応しなくなりました", "%1 不再响应 MIDI", "%1이(가) 더 이상 MIDI에 반응하지 않습니다", "%1 अब MIDI पर प्रतिक्रिया नहीं देगा", "%1 لم يعد يستجيب لـ MIDI", "%1 আর MIDI-তে সাড়া দেবে না", "%1 больше не реагирует на MIDI", "%1 tidak lagi merespons MIDI", "%1 artık MIDI'ye yanıt vermiyor"),
    ("CC %1 ligado. Pronto para tocar.", "CC %1 linked. Ready to play.", "CC %1 enlazado. Listo para tocar.", "CC %1 lié. Prêt à jouer.", "CC %1 collegato. Pronto a suonare.", "CC %1 verbunden. Bereit zum Spielen.", "CC %1 を割り当てました。演奏できます。", "CC %1 已绑定，可以演奏。", "CC %1 연결됨. 연주할 준비가 되었습니다.", "CC %1 जुड़ गया। बजाने के लिए तैयार।", "تم ربط CC %1. جاهز للعزف.", "CC %1 যুক্ত হয়েছে। বাজানোর জন্য প্রস্তুত।", "CC %1 привязан. Можно играть.", "CC %1 tertaut. Siap dimainkan.", "CC %1 bağlandı. Çalmaya hazır."),
    ("MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI", "MIDI"),
    # Títulos e situação
    ("trocar pacote", "change pack", "cambiar paquete", "changer de pack", "cambia pacchetto", "Paket wechseln", "パックを変更", "更换包", "팩 변경", "पैक बदलें", "تغيير الحزمة", "প্যাক বদলান", "сменить пакет", "ganti paket", "paketi değiştir"),
    ("abrir pacote", "open pack", "abrir paquete", "ouvrir un pack", "apri pacchetto", "Paket öffnen", "パックを開く", "打开包", "팩 열기", "पैक खोलें", "فتح حزمة", "প্যাক খুলুন", "открыть пакет", "buka paket", "paket aç"),
    ("abrir pacote do pedal", "open pedal pack", "abrir paquete del pedal", "ouvrir le pack de la pédale", "apri pacchetto del pedale", "Pedal-Paket öffnen", "ペダルのパックを開く", "打开效果器包", "페달 팩 열기", "पेडल पैक खोलें", "فتح حزمة الدوّاسة", "প্যাডেল প্যাক খুলুন", "открыть пакет педали", "buka paket pedal", "pedal paketini aç"),
    ("Caixa incluída na captura (full rig): o IR fica desligado para respeitar o som original", "Cabinet included in the capture (full rig): the IR stays off to respect the original sound", "Caja incluida en la captura (full rig): el IR queda apagado para respetar el sonido original", "Baffle inclus dans la capture (full rig) : l'IR reste désactivée pour respecter le son d'origine", "Cassa inclusa nella cattura (full rig): l'IR resta spenta per rispettare il suono originale", "Box in der Capture enthalten (Full Rig): die IR bleibt aus, um den Originalklang zu wahren", "キャビネットはキャプチャーに含まれています（フルリグ）：原音を保つため IR はオフです", "采样已包含箱体（全套）：为保留原声，IR 保持关闭", "캐비닛이 캡처에 포함됨 (풀 리그): 원래 소리를 지키기 위해 IR은 꺼져 있습니다", "कैबिनेट कैप्चर में शामिल है (फुल रिग): मूल आवाज़ बनाए रखने के लिए IR बंद रहता है", "الصندوق مضمّن في الالتقاط (full rig): يبقى IR متوقفًا للحفاظ على الصوت الأصلي", "ক্যাবিনেট ক্যাপচারে আছে (ফুল রিগ): আসল শব্দ রাখতে IR বন্ধ থাকে", "Кабинет входит в захват (full rig): IR выключен, чтобы сохранить исходный звук", "Kabinet sudah ada dalam tangkapan (full rig): IR dimatikan agar suara asli terjaga", "Kabin yakalamaya dahil (full rig): orijinal sesi korumak için IR kapalı kalır"),
    ("Nenhum IR escolhido", "No IR chosen", "Ningún IR elegido", "Aucune IR choisie", "Nessuna IR scelta", "Keine IR gewählt", "IR が選ばれていません", "未选择 IR", "선택된 IR 없음", "कोई IR नहीं चुना", "لم يتم اختيار IR", "কোনো IR বেছে নেওয়া হয়নি", "IR не выбран", "Belum ada IR", "IR seçilmedi"),
    ("trocar IR", "change IR", "cambiar IR", "changer d'IR", "cambia IR", "IR wechseln", "IR を変更", "更换 IR", "IR 변경", "IR बदलें", "تغيير IR", "IR বদলান", "сменить IR", "ganti IR", "IR'ı değiştir"),
    ("escolher IR", "choose IR", "elegir IR", "choisir une IR", "scegli IR", "IR wählen", "IR を選択", "选择 IR", "IR 선택", "IR चुनें", "اختيار IR", "IR বেছে নিন", "выбрать IR", "pilih IR", "IR seç"),
    ("Carregando...", "Loading...", "Cargando...", "Chargement...", "Caricamento...", "Wird geladen...", "読み込み中...", "正在加载...", "불러오는 중...", "लोड हो रहा है...", "جارٍ التحميل...", "লোড হচ্ছে...", "Загрузка...", "Memuat...", "Yükleniyor..."),
    ("Nenhuma captura no pedal. O pisante liga e desliga.", "No capture on the pedal. The footswitch turns it on and off.", "Ninguna captura en el pedal. El pulsador lo enciende y apaga.", "Aucune capture dans la pédale. Le footswitch l'allume et l'éteint.", "Nessuna cattura nel pedale. Il footswitch lo accende e spegne.", "Keine Capture im Pedal. Der Fußschalter schaltet ein und aus.", "ペダルにキャプチャーがありません。フットスイッチでオン/オフ。", "效果器中没有采样。脚踏开关控制开关。", "페달에 캡처가 없습니다. 풋스위치로 켜고 끕니다.", "पेडल में कोई कैप्चर नहीं। फ़ुटस्विच से चालू/बंद करें।", "لا يوجد التقاط في الدوّاسة. مفتاح القدم يشغّلها ويوقفها.", "প্যাডেলে কোনো ক্যাপচার নেই। ফুটসুইচে চালু/বন্ধ।", "В педали нет захвата. Футсвич включает и выключает её.", "Belum ada tangkapan di pedal. Footswitch menyalakan dan mematikan.", "Pedalde yakalama yok. Ayak anahtarı açıp kapatır."),
    ("Nenhum pacote de capturas carregado", "No capture pack loaded", "Ningún paquete de capturas cargado", "Aucun pack de captures chargé", "Nessun pacchetto di catture caricato", "Kein Capture-Paket geladen", "キャプチャーパックが読み込まれていません", "未加载采样包", "불러온 캡처 팩이 없습니다", "कोई कैप्चर पैक लोड नहीं है", "لم تُحمّل أي حزمة التقاطات", "কোনো ক্যাপচার প্যাক লোড হয়নি", "Пакет захватов не загружен", "Belum ada paket tangkapan", "Yakalama paketi yüklenmedi"),
    ("%1: %2 capturas, ganho de %3 a %4 (%5)", "%1: %2 captures, gain %3 to %4 (%5)", "%1: %2 capturas, ganancia de %3 a %4 (%5)", "%1 : %2 captures, gain de %3 à %4 (%5)", "%1: %2 catture, gain da %3 a %4 (%5)", "%1: %2 Captures, Gain %3 bis %4 (%5)", "%1：キャプチャー %2 個、ゲイン %3〜%4（%5）", "%1：%2 个采样，增益 %3 至 %4（%5）", "%1: 캡처 %2개, 게인 %3~%4 (%5)", "%1: %2 कैप्चर, गेन %3 से %4 (%5)", "%1: %2 التقاطات، الكسب من %3 إلى %4 (%5)", "%1: %2টি ক্যাপচার, গেইন %3 থেকে %4 (%5)", "%1: захватов %2, гейн от %3 до %4 (%5)", "%1: %2 tangkapan, gain %3 sampai %4 (%5)", "%1: %2 yakalama, gain %3 ile %4 arası (%5)"),
    # Dicas da barra de cima
    ("Botões em degraus: param só nas posições capturadas", "Stepped knobs: stop only at captured positions", "Perillas por pasos: solo en las posiciones capturadas", "Boutons crantés : seulement aux positions capturées", "Manopole a scatti: solo nelle posizioni catturate", "Gerasterte Regler: nur an den erfassten Positionen", "ステップ式ノブ：キャプチャー位置でのみ止まる", "步进旋钮：只停在采样位置", "단계식 노브: 캡처된 위치에서만 멈춤", "स्टेप वाले नॉब: केवल कैप्चर की गई जगहों पर रुकते हैं", "مقابض متدرجة: تتوقف عند المواضع الملتقطة فقط", "ধাপে ধাপে নব: শুধু ক্যাপচার করা জায়গায় থামে", "Ступенчатые ручки: только на позициях захватов", "Kenop bertahap: hanya berhenti di posisi tangkapan", "Kademeli düğmeler: yalnızca yakalanan konumlarda durur"),
    ("Sobre o Nexus Modeler", "About Nexus Modeler", "Acerca de Nexus Modeler", "À propos de Nexus Modeler", "Informazioni su Nexus Modeler", "Über Nexus Modeler", "Nexus Modeler について", "关于 Nexus Modeler", "Nexus Modeler 정보", "Nexus Modeler के बारे में", "حول Nexus Modeler", "Nexus Modeler সম্পর্কে", "О Nexus Modeler", "Tentang Nexus Modeler", "Nexus Modeler hakkında"),
    ("Interface de áudio", "Audio interface", "Interfaz de audio", "Interface audio", "Interfaccia audio", "Audio-Interface", "オーディオインターフェース", "音频接口", "오디오 인터페이스", "ऑडियो इंटरफ़ेस", "واجهة الصوت", "অডিও ইন্টারফেস", "Аудиоинтерфейс", "Antarmuka audio", "Ses arabirimi"),
    ("Idioma", "Language", "Idioma", "Langue", "Lingua", "Sprache", "言語", "语言", "언어", "भाषा", "اللغة", "ভাষা", "Язык", "Bahasa", "Dil"),
    ("Visual anterior do pedal", "Previous pedal look", "Aspecto anterior del pedal", "Apparence précédente de la pédale", "Aspetto precedente del pedale", "Vorheriges Pedal-Design", "前のペダルデザイン", "上一个效果器外观", "이전 페달 디자인", "पिछला पेडल रूप", "مظهر الدوّاسة السابق", "আগের প্যাডেল চেহারা", "Предыдущий вид педали", "Tampilan pedal sebelumnya", "Önceki pedal görünümü"),
    ("Próximo visual do pedal", "Next pedal look", "Siguiente aspecto del pedal", "Apparence suivante de la pédale", "Aspetto successivo del pedale", "Nächstes Pedal-Design", "次のペダルデザイン", "下一个效果器外观", "다음 페달 디자인", "अगला पेडल रूप", "مظهر الدوّاسة التالي", "পরের প্যাডেল চেহারা", "Следующий вид педали", "Tampilan pedal berikutnya", "Sonraki pedal görünümü"),
    ("visual ainda não renderizado", "look not rendered yet", "aspecto aún no renderizado", "apparence pas encore rendue", "aspetto non ancora renderizzato", "Design noch nicht gerendert", "デザインはまだレンダリングされていません", "外观尚未渲染", "디자인이 아직 렌더링되지 않음", "रूप अभी रेंडर नहीं हुआ", "المظهر لم يُعرض بعد", "চেহারা এখনো রেন্ডার হয়নি", "вид ещё не отрисован", "tampilan belum dirender", "görünüm henüz oluşturulmadı"),
    # Painel de áudio
    ("Sistema de áudio", "Audio system", "Sistema de audio", "Système audio", "Sistema audio", "Audiosystem", "オーディオシステム", "音频系统", "오디오 시스템", "ऑडियो सिस्टम", "نظام الصوت", "অডিও সিস্টেম", "Аудиосистема", "Sistem audio", "Ses sistemi"),
    ("Interface", "Interface", "Interfaz", "Interface", "Interfaccia", "Interface", "インターフェース", "接口", "인터페이스", "इंटरफ़ेस", "الواجهة", "ইন্টারফেস", "Интерфейс", "Antarmuka", "Arabirim"),
    ("Interface de entrada", "Input interface", "Interfaz de entrada", "Interface d'entrée", "Interfaccia di ingresso", "Eingangs-Interface", "入力インターフェース", "输入接口", "입력 인터페이스", "इनपुट इंटरफ़ेस", "واجهة الدخل", "ইনপুট ইন্টারফেস", "Интерфейс входа", "Antarmuka masukan", "Giriş arabirimi"),
    ("Interface de saída", "Output interface", "Interfaz de salida", "Interface de sortie", "Interfaccia di uscita", "Ausgangs-Interface", "出力インターフェース", "输出接口", "출력 인터페이스", "आउटपुट इंटरफ़ेस", "واجهة الخرج", "আউটপুট ইন্টারফেস", "Интерфейс выхода", "Antarmuka keluaran", "Çıkış arabirimi"),
    ("Entrada da guitarra", "Guitar input", "Entrada de la guitarra", "Entrée guitare", "Ingresso chitarra", "Gitarreneingang", "ギター入力", "吉他输入", "기타 입력", "गिटार इनपुट", "دخل الجيتار", "গিটার ইনপুট", "Вход гитары", "Masukan gitar", "Gitar girişi"),
    ("Saídas", "Outputs", "Salidas", "Sorties", "Uscite", "Ausgänge", "出力", "输出", "출력", "आउटपुट", "المخارج", "আউটপুট", "Выходы", "Keluaran", "Çıkışlar"),
    ("Taxa", "Sample rate", "Frecuencia", "Fréquence", "Frequenza", "Abtastrate", "サンプルレート", "采样率", "샘플레이트", "सैंपल रेट", "معدل العينات", "স্যাম্পল রেট", "Частота", "Laju sampel", "Örnekleme hızı"),
    ("Buffer", "Buffer", "Búfer", "Tampon", "Buffer", "Puffer", "バッファー", "缓冲区", "버퍼", "बफ़र", "المخزن المؤقت", "বাফার", "Буфер", "Buffer", "Arabellek"),
    ("%1 amostras", "%1 samples", "%1 muestras", "%1 échantillons", "%1 campioni", "%1 Samples", "%1 サンプル", "%1 个采样点", "%1 샘플", "%1 सैंपल", "%1 عينة", "%1 স্যাম্পল", "%1 сэмплов", "%1 sampel", "%1 örnek"),
    ("(recomendada: sem conversão)", "(recommended: no conversion)", "(recomendada: sin conversión)", "(recommandée : sans conversion)", "(consigliata: nessuna conversione)", "(empfohlen: keine Umrechnung)", "（推奨：変換なし）", "（推荐：无需转换）", "(권장: 변환 없음)", "(सुझाया गया: कोई रूपांतरण नहीं)", "(موصى به: دون تحويل)", "(প্রস্তাবিত: রূপান্তর নেই)", "(рекомендуется: без преобразования)", "(disarankan: tanpa konversi)", "(önerilen: dönüştürme yok)"),
    ("Painel do driver", "Driver panel", "Panel del controlador", "Panneau du pilote", "Pannello del driver", "Treiber-Panel", "ドライバーパネル", "驱动面板", "드라이버 패널", "ड्राइवर पैनल", "لوحة التعريف", "ড্রাইভার প্যানেল", "Панель драйвера", "Panel driver", "Sürücü paneli"),
    ("Parear MIDI Bluetooth", "Pair Bluetooth MIDI", "Emparejar MIDI Bluetooth", "Associer un MIDI Bluetooth", "Associa MIDI Bluetooth", "Bluetooth-MIDI koppeln", "Bluetooth MIDI をペアリング", "配对蓝牙 MIDI", "블루투스 MIDI 페어링", "ब्लूटूथ MIDI जोड़ें", "إقران MIDI عبر بلوتوث", "ব্লুটুথ MIDI জুড়ুন", "Подключить Bluetooth MIDI", "Pasangkan MIDI Bluetooth", "Bluetooth MIDI eşleştir"),
    ("Fechar", "Close", "Cerrar", "Fermer", "Chiudi", "Schließen", "閉じる", "关闭", "닫기", "बंद करें", "إغلاق", "বন্ধ করুন", "Закрыть", "Tutup", "Kapat"),
    ("Latência ida e volta", "Round-trip latency", "Latencia ida y vuelta", "Latence aller-retour", "Latenza andata e ritorno", "Roundtrip-Latenz", "往復レイテンシー", "往返延迟", "왕복 지연", "राउंड-ट्रिप लेटेंसी", "زمن الاستجابة ذهابًا وإيابًا", "রাউন্ড-ট্রিপ লেটেন্সি", "Задержка туда и обратно", "Latensi pulang-pergi", "Gidiş-dönüş gecikmesi"),
    ("sem interface", "no interface", "sin interfaz", "aucune interface", "nessuna interfaccia", "kein Interface", "インターフェースなし", "无接口", "인터페이스 없음", "कोई इंटरफ़ेस नहीं", "لا توجد واجهة", "ইন্টারফেস নেই", "нет интерфейса", "tanpa antarmuka", "arabirim yok"),
    ("Sinal da guitarra", "Guitar signal", "Señal de la guitarra", "Signal de la guitare", "Segnale della chitarra", "Gitarrensignal", "ギター信号", "吉他信号", "기타 신호", "गिटार सिग्नल", "إشارة الجيتار", "গিটার সিগন্যাল", "Сигнал гитары", "Sinyal gitar", "Gitar sinyali"),
    ("Processador: %1%", "CPU: %1%", "Procesador: %1%", "Processeur : %1%", "Processore: %1%", "Prozessor: %1%", "CPU：%1%", "处理器：%1%", "프로세서: %1%", "प्रोसेसर: %1%", "المعالج: %1%", "প্রসেসর: %1%", "Процессор: %1%", "Prosesor: %1%", "İşlemci: %1%"),
    ("nenhum dispositivo conectado", "no device connected", "ningún dispositivo conectado", "aucun appareil connecté", "nessun dispositivo collegato", "kein Gerät verbunden", "接続されたデバイスはありません", "未连接设备", "연결된 장치 없음", "कोई डिवाइस जुड़ा नहीं", "لا يوجد جهاز متصل", "কোনো ডিভাইস যুক্ত নেই", "устройства не подключены", "tidak ada perangkat", "bağlı cihaz yok"),
]


def escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main() -> None:
    out = Path(__file__).resolve().parents[2] / "assets" / "idiomas"
    out.mkdir(parents=True, exist_ok=True)
    for i, (code, name, countries) in enumerate(IDIOMAS):
        lines = [f"language: {name}", f"countries: {countries}", ""]
        for row in TABELA:
            assert len(row) == 1 + len(IDIOMAS), f"linha incompleta: {row[0]}"
            lines.append(f'"{escape(row[0])}" = "{escape(row[1 + i])}"')
        (out / f"idioma_{code}.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"{len(IDIOMAS)} idiomas, {len(TABELA)} frases em {out}")


if __name__ == "__main__":
    main()
