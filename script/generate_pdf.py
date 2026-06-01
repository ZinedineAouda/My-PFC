import os
import sys
import math
from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import HexColor, white, black
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_JUSTIFY
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether
from reportlab.pdfgen import canvas
from reportlab.graphics.shapes import Drawing, Rect, String, Line, Polygon, Group

# Colors
PRIMARY_COLOR = HexColor("#1A365D")  # Deep Navy Blue
SECONDARY_COLOR = HexColor("#2B6CB0")  # Royal Blue
TEXT_COLOR = HexColor("#2D3748")  # Dark Slate Grey
MUTED_TEXT = HexColor("#718096")  # Warm Grey
BG_LIGHT = HexColor("#F7FAFC")  # Light Grey for tables and blocks
BORDER_COLOR = HexColor("#E2E8F0")  # Light border color

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_elements(num_pages)
            super().showPage()
        super().save()

    def draw_page_elements(self, page_count):
        # We do NOT want headers/footers on page 1 (cover page)
        if self._pageNumber == 1:
            return
        
        self.saveState()
        self.setFont("Helvetica", 8)
        self.setFillColor(MUTED_TEXT)
        
        # Header text
        self.drawString(54, 805, "Système d'Alerte Médicale pour Patients Hospitalisés — Rapport Technique")
        # Header line
        self.setStrokeColor(BORDER_COLOR)
        self.setLineWidth(0.5)
        self.line(54, 797, 541.27, 797) # A4 width is 595.27. Left margin 54, right margin 54. Width = 487.27
        
        # Footer line
        self.line(54, 50, 541.27, 50)
        # Footer text
        self.drawString(54, 35, "Projet de Fin de Cycle (PFC)")
        page_text = f"Page {self._pageNumber} sur {page_count}"
        self.drawRightString(541.27, 35, page_text)
        self.restoreState()

def draw_line_with_arrow(d, x1, y1, x2, y2, color=HexColor("#4A5568"), width=1.5, double=False):
    d.add(Line(x1, y1, x2, y2, strokeColor=color, strokeWidth=width))
    
    # Calculate arrowhead direction
    dx = x2 - x1
    dy = y2 - y1
    length = math.sqrt(dx*dx + dy*dy)
    if length > 0:
        ux = dx / length
        uy = dy / length
        arrow_len = 6
        arrow_width = 3.5
        px = -uy
        py = ux
        
        # Arrowhead at end (x2, y2)
        ax = x2 - arrow_len * ux
        ay = y2 - arrow_len * uy
        p1x = ax + arrow_width * px
        p1y = ay + arrow_width * py
        p2x = ax - arrow_width * px
        p2y = ay - arrow_width * py
        d.add(Polygon([x2, y2, p1x, p1y, p2x, p2y], fillColor=color, strokeColor=color))
        
        # Optional arrowhead at start (x1, y1) for bidirectionality
        if double:
            bx = x1 + arrow_len * ux
            by = y1 + arrow_len * uy
            p3x = bx + arrow_width * px
            p3y = by + arrow_width * py
            p4x = bx - arrow_width * px
            p4y = by - arrow_width * py
            d.add(Polygon([x1, y1, p3x, p3y, p4x, p4y], fillColor=color, strokeColor=color))

def draw_architecture_diagram():
    # A4 printable area width is 487.27
    dw = 487
    dh = 240
    d = Drawing(dw, dh)
    
    # Border wrapper for diagram
    d.add(Rect(0, 0, dw, dh, fillColor=BG_LIGHT, strokeColor=BORDER_COLOR, strokeWidth=1, rx=6, ry=6))
    
    # 1. Cloud Platform Box (Top)
    cx, cy, cw, ch = 103, 160, 280, 65
    d.add(Rect(cx, cy, cw, ch, fillColor=HexColor("#EBF8FF"), strokeColor=SECONDARY_COLOR, strokeWidth=1.5, rx=4, ry=4))
    d.add(String(cx + cw/2, cy + 47, "CLOUD PLATFORM (Railway Monolith)", fontName="Helvetica-Bold", fontSize=9, textAnchor="middle", fillColor=PRIMARY_COLOR))
    d.add(String(cx + cw/2, cy + 32, "• Express API & WebSockets Server", fontName="Helvetica", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    d.add(String(cx + cw/2, cy + 20, "• Embedded Aedes MQTT Broker (Port 1883)", fontName="Helvetica", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    d.add(String(cx + cw/2, cy + 8, "• PostgreSQL Database via Drizzle ORM", fontName="Helvetica", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    
    # 2. ESP32-S3 Controller Box (Middle)
    ex, ey, ew, eh = 143, 85, 200, 45
    d.add(Rect(ex, ey, ew, eh, fillColor=HexColor("#FEFCBF"), strokeColor=HexColor("#D69E2E"), strokeWidth=1.5, rx=4, ry=4))
    d.add(String(ex + ew/2, ey + 30, "ESP32-S3 CONTROLLER", fontName="Helvetica-Bold", fontSize=9, textAnchor="middle", fillColor=HexColor("#744210")))
    d.add(String(ex + ew/2, ey + 18, "• Passerelle IoT Locale (AP / STA)", fontName="Helvetica", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    d.add(String(ex + ew/2, ey + 6, "• Buzzer d'Alarme Physique (GPIO 4)", fontName="Helvetica", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    
    # 3. ESP8266 Devices (Bottom Row)
    dev_y = 15
    dev_w, dev_h = 95, 35
    
    # Device 1
    d.add(Rect(40, dev_y, dev_w, dev_h, fillColor=HexColor("#EDF2F7"), strokeColor=HexColor("#4A5568"), strokeWidth=1.2, rx=3, ry=3))
    d.add(String(40 + dev_w/2, dev_y + 22, "ESP8266 Terminal", fontName="Helvetica-Bold", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    d.add(String(40 + dev_w/2, dev_y + 10, "Lit Patient 1 (Ch. 101)", fontName="Helvetica", fontSize=7.5, textAnchor="middle", fillColor=MUTED_TEXT))
    
    # Device 2
    d.add(Rect(196, dev_y, dev_w, dev_h, fillColor=HexColor("#EDF2F7"), strokeColor=HexColor("#4A5568"), strokeWidth=1.2, rx=3, ry=3))
    d.add(String(196 + dev_w/2, dev_y + 22, "ESP8266 Terminal", fontName="Helvetica-Bold", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    d.add(String(196 + dev_w/2, dev_y + 10, "Lit Patient 2 (Ch. 102)", fontName="Helvetica", fontSize=7.5, textAnchor="middle", fillColor=MUTED_TEXT))
    
    # Device 3
    d.add(Rect(352, dev_y, dev_w, dev_h, fillColor=HexColor("#EDF2F7"), strokeColor=HexColor("#4A5568"), strokeWidth=1.2, rx=3, ry=3))
    d.add(String(352 + dev_w/2, dev_y + 22, "ESP8266 Terminal", fontName="Helvetica-Bold", fontSize=8, textAnchor="middle", fillColor=TEXT_COLOR))
    d.add(String(352 + dev_w/2, dev_y + 10, "Lit Patient N (Ch. 10N)", fontName="Helvetica", fontSize=7.5, textAnchor="middle", fillColor=MUTED_TEXT))
    
    # 4. Connections & Arrows
    # Connections: ESP8266 -> ESP32-S3
    draw_line_with_arrow(d, 40 + dev_w/2, dev_y + dev_h, ex + 25, ey, color=HexColor("#4A5568"), width=1.2, double=True)
    draw_line_with_arrow(d, 196 + dev_w/2, dev_y + dev_h, ex + ew/2, ey, color=HexColor("#4A5568"), width=1.2, double=True)
    draw_line_with_arrow(d, 352 + dev_w/2, dev_y + dev_h, ex + ew - 25, ey, color=HexColor("#4A5568"), width=1.2, double=True)
    
    # Labels for local network
    d.add(String(85, 60, "MQTT (Local / Wi-Fi)", fontName="Helvetica-Oblique", fontSize=6.5, fillColor=MUTED_TEXT, textAnchor="middle"))
    d.add(String(402, 60, "MQTT (Local / Wi-Fi)", fontName="Helvetica-Oblique", fontSize=6.5, fillColor=MUTED_TEXT, textAnchor="middle"))
    
    # Connection: ESP32-S3 -> Cloud
    draw_line_with_arrow(d, ex + ew/2, ey + eh, cx + cw/2, cy, color=SECONDARY_COLOR, width=1.5, double=True)
    d.add(String(cx + cw/2 + 8, ey + eh + 15, "HTTPS API & WebSockets & MQTT (Port 1883)", fontName="Helvetica-Bold", fontSize=7, fillColor=SECONDARY_COLOR, textAnchor="start"))
    
    # Extra: Direct Dashboards (Web / Mobile app)
    dash_x, dash_y, dash_w, dash_h = 395, 100, 80, 40
    d.add(Rect(dash_x, dash_y, dash_w, dash_h, fillColor=HexColor("#E2E8F0"), strokeColor=MUTED_TEXT, strokeWidth=1, rx=3, ry=3))
    d.add(String(dash_x + dash_w/2, dash_y + 26, "DASHBOARDS", fontName="Helvetica-Bold", fontSize=8, textAnchor="middle", fillColor=PRIMARY_COLOR))
    d.add(String(dash_x + dash_w/2, dash_y + 16, "React Web Client", fontName="Helvetica", fontSize=7, textAnchor="middle", fillColor=TEXT_COLOR))
    d.add(String(dash_x + dash_w/2, dash_y + 6, "Capacitor Mobile", fontName="Helvetica", fontSize=7, textAnchor="middle", fillColor=TEXT_COLOR))
    
    # Dashboard connection to Cloud
    draw_line_with_arrow(d, dash_x, dash_y + dash_h/2, cx + cw - 10, cy + ch/2, color=HexColor("#319795"), width=1.2, double=True)
    d.add(String(dash_x - 5, dash_y + dash_h/2 + 10, "WSS / HTTPS", fontName="Helvetica-Bold", fontSize=7, fillColor=HexColor("#319795"), textAnchor="end"))
    
    return d

def create_code_block(styles, code_text):
    escaped_code = code_text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\n", "<br/>")
    p = Paragraph(f"<font name='Courier' size='7.5' color='#1a202c'>{escaped_code}</font>", styles['CodeText'])
    t = Table([[p]], colWidths=[487])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), BG_LIGHT),
        ('BOX', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('LEFTPADDING', (0,0), (-1,-1), 10),
        ('RIGHTPADDING', (0,0), (-1,-1), 10),
        ('TOPPADDING', (0,0), (-1,-1), 8),
        ('BOTTOMPADDING', (0,0), (-1,-1), 8),
    ]))
    return t

def create_styled_table(styles, data, col_widths=None):
    header_style = ParagraphStyle(
        'HeaderStyle',
        fontName='Helvetica-Bold',
        fontSize=8.5,
        leading=11,
        textColor=white
    )
    cell_style = ParagraphStyle(
        'CellStyle',
        fontName='Helvetica',
        fontSize=8,
        leading=10.5,
        textColor=TEXT_COLOR
    )
    
    formatted_data = []
    # Header row
    header_row = [Paragraph(f"<b>{cell}</b>", header_style) for cell in data[0]]
    formatted_data.append(header_row)
    
    # Body rows
    for row in data[1:]:
        formatted_row = [Paragraph(str(cell), cell_style) for cell in row]
        formatted_data.append(formatted_row)
        
    t = Table(formatted_data, colWidths=col_widths)
    
    t_style = [
        ('BACKGROUND', (0,0), (-1,0), PRIMARY_COLOR),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('BOTTOMPADDING', (0,0), (-1,-1), 5),
        ('TOPPADDING', (0,0), (-1,-1), 5),
        ('LEFTPADDING', (0,0), (-1,-1), 7),
        ('RIGHTPADDING', (0,0), (-1,-1), 7),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
    ]
    
    for i in range(1, len(data)):
        if i % 2 == 0:
            t_style.append(('BACKGROUND', (0,i), (-1,i), BG_LIGHT))
            
    t.setStyle(TableStyle(t_style))
    return t

def build_pdf(filename):
    # Setup document template
    # Margins: 54 pt = 0.75 inch (~1.9 cm). Height of A4 is 841.89, Width is 595.27.
    # Printable area: Width = 487.27, Height = 697.89.
    doc = SimpleDocTemplate(
        filename,
        pagesize=A4,
        leftMargin=54,
        rightMargin=54,
        topMargin=72,
        bottomMargin=72
    )
    
    styles = getSampleStyleSheet()
    
    # Custom Typography Styles
    title_style = ParagraphStyle(
        'CoverTitle',
        fontName='Helvetica-Bold',
        fontSize=25,
        leading=32,
        alignment=TA_CENTER,
        textColor=PRIMARY_COLOR,
        spaceAfter=15
    )
    subtitle_style = ParagraphStyle(
        'CoverSubtitle',
        fontName='Helvetica',
        fontSize=13,
        leading=18,
        alignment=TA_CENTER,
        textColor=SECONDARY_COLOR,
        spaceAfter=25
    )
    metadata_label = ParagraphStyle(
        'MetaLabel',
        fontName='Helvetica-Bold',
        fontSize=10,
        leading=14,
        textColor=PRIMARY_COLOR
    )
    metadata_value = ParagraphStyle(
        'MetaValue',
        fontName='Helvetica',
        fontSize=10,
        leading=14,
        textColor=TEXT_COLOR
    )
    
    h1_style = ParagraphStyle(
        'Heading1_Custom',
        fontName='Helvetica-Bold',
        fontSize=17,
        leading=21,
        textColor=PRIMARY_COLOR,
        spaceBefore=18,
        spaceAfter=10,
        keepWithNext=True
    )
    
    h2_style = ParagraphStyle(
        'Heading2_Custom',
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=SECONDARY_COLOR,
        spaceBefore=14,
        spaceAfter=6,
        keepWithNext=True
    )
    
    h3_style = ParagraphStyle(
        'Heading3_Custom',
        fontName='Helvetica-Bold',
        fontSize=9.5,
        leading=13,
        textColor=TEXT_COLOR,
        spaceBefore=10,
        spaceAfter=4,
        keepWithNext=True
    )
    
    body_style = ParagraphStyle(
        'Body_Custom',
        fontName='Helvetica',
        fontSize=9.5,
        leading=13.5,
        textColor=TEXT_COLOR,
        spaceAfter=8,
        alignment=TA_JUSTIFY
    )
    
    bullet_style = ParagraphStyle(
        'Bullet_Custom',
        fontName='Helvetica',
        fontSize=9.5,
        leading=13.5,
        textColor=TEXT_COLOR,
        leftIndent=15,
        firstLineIndent=-10,
        spaceAfter=5
    )
    
    styles.add(ParagraphStyle('CodeText', fontName='Courier', fontSize=7.5, leading=10, textColor=HexColor("#1A202C")))
    
    story = []
    
    # ---------------- PAGE 1: COVER PAGE ----------------
    story.append(Spacer(1, 40))
    # Elegant top line
    top_bar = Table([[""]], colWidths=[487], rowHeights=[4])
    top_bar.setStyle(TableStyle([('BACKGROUND', (0,0), (-1,-1), PRIMARY_COLOR)]))
    story.append(top_bar)
    story.append(Spacer(1, 40))
    
    # Institution / Course Header
    story.append(Paragraph("<font size='10' color='#718096'><b>PROJET DE FIN DE CYCLE (PFC)</b></font>", ParagraphStyle('HeaderInst', fontName='Helvetica-Bold', alignment=TA_CENTER, spaceAfter=20)))
    story.append(Spacer(1, 20))
    
    # Main Title
    story.append(Paragraph("SYSTÈME D'ALERTE MÉDICALE CONNECTÉ", title_style))
    story.append(Paragraph("Hospital Patient Alarm System", subtitle_style))
    
    story.append(Spacer(1, 15))
    
    # Decorative horizontal line
    mid_bar = Table([[""]], colWidths=[120], rowHeights=[1.5])
    mid_bar.setStyle(TableStyle([('BACKGROUND', (0,0), (-1,-1), SECONDARY_COLOR), ('ALIGN', (0,0), (-1,-1), 'CENTER')]))
    story.append(mid_bar)
    
    story.append(Spacer(1, 40))
    
    # Description block
    story.append(Paragraph(
        "Rapport de documentation technique approfondie décrivant la conception matérielle, "
        "l'architecture logicielle réseau (Node.js & Aedes MQTT), la structure de données "
        "PostgreSQL (Drizzle ORM) et l'intégration mobile via Capacitor WebView.",
        ParagraphStyle('DescBlock', fontName='Helvetica-Oblique', fontSize=10, leading=15, alignment=TA_CENTER, textColor=TEXT_COLOR, leftIndent=40, rightIndent=40)
    ))
    
    story.append(Spacer(1, 120))
    
    # Metadata Box at the bottom
    meta_data = [
        [Paragraph("Auteur :", metadata_label), Paragraph("Zinedine Aouda", metadata_value)],
        [Paragraph("Rôle :", metadata_label), Paragraph("Ingénieur Concepteur de Système IoT & Logiciel", metadata_value)],
        [Paragraph("Base de données :", metadata_label), Paragraph("PostgreSQL + Drizzle ORM", metadata_value)],
        [Paragraph("Microcontrôleurs :", metadata_label), Paragraph("ESP32-S3 (Contrôleur) & ESP8266 (Terminaux)", metadata_value)],
        [Paragraph("Date :", metadata_label), Paragraph("Mai 2026", metadata_value)],
        [Paragraph("Version :", metadata_label), Paragraph("2.0.0 (Cloud Monolith Migration)", metadata_value)]
    ]
    meta_table = Table(meta_data, colWidths=[130, 250])
    meta_table.setStyle(TableStyle([
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('LEFTPADDING', (0,0), (-1,-1), 0),
        ('LINEBELOW', (0,0), (-1,-2), 0.5, BORDER_COLOR),
    ]))
    
    # Wrap in a simple box layout to center it
    meta_container = Table([[meta_table]], colWidths=[380])
    meta_container.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), BG_LIGHT),
        ('BOX', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('PADDING', (0,0), (-1,-1), 15),
        ('ALIGN', (0,0), (-1,-1), 'CENTER')
    ]))
    story.append(meta_container)
    
    story.append(PageBreak())
    
    # ---------------- PAGE 2: INTRODUCTION ----------------
    story.append(Paragraph("1. Introduction Générale", h1_style))
    story.append(Paragraph(
        "Le présent projet consiste en la conception et la réalisation d'un <b>système d'alerte médicale "
        "connecté (Hospital Patient Alarm System)</b>. Ce dispositif a pour objectif principal d'optimiser "
        "la réactivité et la gestion des appels d'urgence au sein des établissements de santé. Il permet aux patients "
        "hospitalisés d'émettre des alertes instantanées depuis leur lit vers un poste de surveillance centralisé "
        "ou directement sur les terminaux mobiles du personnel soignant (infirmiers de garde).",
        body_style
    ))
    
    story.append(Paragraph("1.1 Problématique", h2_style))
    story.append(Paragraph(
        "Dans la configuration hospitalière traditionnelle, les systèmes d'appel d'urgence physiques souffrent "
        "souvent de plusieurs limites critiques :",
        body_style
    ))
    
    story.append(Paragraph("• <b>Coût et rigidité matérielle :</b> L'implémentation de systèmes entièrement filaires nécessite des "
                           "travaux de câblage importants, ce qui augmente considérablement les coûts d'installation et de maintenance.", bullet_style))
    story.append(Paragraph("• <b>Perte de mobilité des infirmiers :</b> Les signaux d'alertes traditionnels sont souvent limités "
                           "à des voyants lumineux de porte ou des moniteurs fixes dans la salle de garde. Les infirmiers en déplacement "
                           "dans le service n'ont aucun moyen d'être avertis en temps réel à moins de retourner au poste central.", bullet_style))
    story.append(Paragraph("• <b>Vulnérabilité face aux coupures de réseau :</b> Les solutions cloud pures exposent le système "
                           "à une panne critique si la connexion Internet externe est perdue. Dans un cadre de santé, une interruption "
                           "de service de quelques minutes peut avoir des conséquences vitales.", bullet_style))
    
    story.append(Paragraph("1.2 Solution Proposée", h2_style))
    story.append(Paragraph(
        "Pour résoudre ces enjeux majeurs de sécurité, de coût et de mobilité, nous avons développé une architecture "
        "connectée hybride et résiliente, reposant sur trois couches coordonnées :",
        body_style
    ))
    
    story.append(Paragraph("1. <b>Une couche matérielle IoT résiliente :</b> Chaque chambre ou lit de patient dispose d'un boîtier "
                           "émetteur <b>ESP8266</b> équipé d'un bouton poussoir. Un contrôleur central intelligent <b>ESP32-S3</b>, agissant "
                           "comme passerelle locale, supervise les boîtiers de sa zone et pilote un buzzer d'alarme physique, garantissant le "
                           "fonctionnement du système localement, même sans aucune liaison Internet.", bullet_style))
    
    story.append(Paragraph("2. <b>Une plateforme Cloud Centralisée (Railway Monolith) :</b> Un serveur Node.js / Express fait office de "
                           "concentrateur global. Il embarque nativement son propre courtier (broker) <b>Aedes MQTT</b> et fournit les flux "
                           "<b>WebSockets</b> temps réel tout en gérant la persistance dans une base de données <b>PostgreSQL</b>.", bullet_style))
    
    story.append(Paragraph("3. <b>Une application Mobile de Réception (Capacitor) :</b> L'application utilisée par le personnel "
                           "soignant est bâtie en React encapsulé dans une WebView native de type <b>Capacitor</b>. Elle reçoit les notifications "
                           "et alertes en temps réel, bénéficie des vibrations matérielles du téléphone, et offre une flexibilité de mise à jour instantanée "
                           "sans recompilation d'APK.", bullet_style))
    
    story.append(Spacer(1, 10))
    
    # ---------------- PAGE 3: ARCHITECTURE GLOBALE ----------------
    story.append(Paragraph("2. Architecture Globale du Système", h1_style))
    story.append(Paragraph(
        "Récemment migré d'une infrastructure éclatée (Vercel Serverless pour l'API, HiveMQ externe pour le MQTT) "
        "vers une architecture <b>Cloud Monolithique centralisée</b>, le système offre désormais des gains majeurs en performance et en "
        "synchronisation. La centralisation du broker MQTT et du serveur de WebSockets au sein du même processus Node.js sur Railway "
        "permet de court-circuiter les latences réseaux.",
        body_style
    ))
    
    story.append(Spacer(1, 5))
    
    # Vector Architecture Diagram
    story.append(Paragraph("<b>Figure 2.1 : Topologie réseau et flux de données de l'architecture hybride</b>", ParagraphStyle('FigCaption', fontName='Helvetica-Bold', fontSize=8.5, leading=11, spaceAfter=8, textColor=PRIMARY_COLOR)))
    story.append(draw_architecture_diagram())
    story.append(Spacer(1, 12))
    
    story.append(Paragraph("2.1 Les Terminaux Patients (ESP8266)", h2_style))
    story.append(Paragraph(
        "Chaque boîtier patient est conçu autour du circuit Wi-Fi low-cost ESP8266. Configuré pour consommer un minimum de courant "
        "au repos, il maintient une liaison MQTT ouverte avec le broker cloud. L'ESP8266 surveille une broche d'interruption "
        "physique reliée au bouton de lit. Lors de l'activation, il bascule sa LED interne à l'état fixe et transmet la commande.",
        body_style
    ))
    
    story.append(Paragraph("2.2 Le Contrôleur Local (ESP32-S3)", h2_style))
    story.append(Paragraph(
        "L'ESP32-S3 sert de passerelle d'agrégation matérielle locale. Il établit une double interface : il peut se "
        "comporter comme un point d'accès Wi-Fi (Mode Access Point) pour les terminaux en l'absence de réseau hôte hospitalier, "
        "ou se connecter au Wi-Fi de l'établissement (Mode Station) pour synchroniser l'ensemble des données locales avec la plateforme Railway. "
        "En cas d'alerte, c'est lui qui déclenche la sirène d'alerte locale branchée sur sa sortie PWM/GPIO.",
        body_style
    ))
    
    story.append(Paragraph("2.3 Le Serveur Cloud Monolithique (Node.js & Aedes)", h2_style))
    story.append(Paragraph(
        "Hébergée sur Railway, l'instance Node.js unifie le routage HTTP, les canaux de communication IoT et la distribution client. "
        "Le protocole MQTT transite par la bibliothèque <b>Aedes</b> configurée en écoute directe sur le port standard 1883. "
        "Lorsqu'un message d'alerte arrive, le broker l'intercepte, l'insère dans la base PostgreSQL via Drizzle ORM, et le transmet simultanément "
        "au serveur WebSocket (WSS) interne pour notifier instantanément les navigateurs et terminaux mobiles ouverts.",
        body_style
    ))
    
    story.append(Paragraph("2.4 Le Dashboard Mobile (React & Capacitor)", h2_style))
    story.append(Paragraph(
        "Plutôt que d'employer une structure React Native complexe nécessitant des compilations régulières pour chaque correction "
        "d'interface, le dashboard mobile repose sur <b>Capacitor</b>. Cette technologie enveloppe la Single Page Application (SPA) React "
        "hébergée sur Railway. Le terminal mobile charge dynamiquement la page cloud tout en gardant accès aux APIs matérielles (vibreur, son de notification).",
        body_style
    ))
    
    # ---------------- PAGE 4: SPECIFICATIONS MATERIELLES ----------------
    story.append(Paragraph("3. Spécifications du Matériel (Hardware)", h1_style))
    story.append(Paragraph(
        "La conception matérielle a été pensée pour être la plus épurée possible afin de maximiser la fiabilité et de "
        "réduire les pannes physiques. L'utilisation de résistances de pull-up internes sur les microcontrôleurs permet d'éliminer "
        "les résistances externes dans le câblage des boutons poussoirs.",
        body_style
    ))
    
    story.append(Paragraph("3.1 Schéma des Connexions & Broches (Pin Mapping)", h2_style))
    
    story.append(Paragraph("A. Contrôleur Central (ESP32-S3 DevKitC-1)", h3_style))
    story.append(Paragraph(
        "L'ESP32-S3 pilote l'avertisseur sonore d'alerte physique. Un buzzer actif est utilisé pour générer un son continu "
        "sans nécessiter de modulation de fréquence complexe dans le code.",
        body_style
    ))
    
    esp32_table_data = [
        ["Broche ESP32-S3", "Composant", "Rôle", "Description / Détail technique"],
        ["GPIO 4", "Buzzer Actif (+)", "Sortie Numérique", "Activé à l'état HAUT (HIGH, 3.3V) pour déclencher l'alarme sonore physique locale."],
        ["GND", "Buzzer Actif (-)", "Masse (0V)", "Masse de référence électrique commune."],
        ["USB-C", "Port Série / Alim", "Alimentation & Debug", "Interface d'alimentation USB 5V et liaison de débogage UART à 115200 Bauds."]
    ]
    story.append(create_styled_table(styles, esp32_table_data, [95, 95, 90, 207]))
    story.append(Spacer(1, 10))
    
    story.append(Paragraph("B. Terminal Bouton Patient (ESP8266 NodeMCU)", h3_style))
    story.append(Paragraph(
        "Chaque boîtier patient possède un unique bouton d'appel d'urgence et utilise sa LED intégrée (souvent située "
        "sur le module ESP-12F) pour confirmer l'état de l'appel au patient hospitalisé.",
        body_style
    ))
    
    esp8266_table_data = [
        ["Broche ESP8266", "Composant", "Mode Configuration", "Description / Détail technique"],
        ["GPIO 0 (D3)", "Bouton Poussoir", "INPUT_PULLUP", "Détecte l'appui. Le bouton relie la broche à la masse (GND). Actif à l'état BAS (LOW)."],
        ["GPIO 2 (D4)", "LED intégrée", "OUTPUT", "LED d'indication. Active à l'état BAS (LOW, 0V) et éteinte à l'état HAUT (HIGH, 3.3V)."]
    ]
    story.append(create_styled_table(styles, esp8266_table_data, [95, 95, 90, 207]))
    story.append(Spacer(1, 12))
    
    story.append(Paragraph("3.2 Comportement Lumineux du Terminal Patient", h2_style))
    story.append(Paragraph(
        "Afin de fournir un retour d'expérience clair et sécurisant aux patients (souvent stressés ou âgés) et aux "
        "techniciens chargés de la maintenance, la LED du boîtier d'appel ESP8266 possède quatre états distincts :",
        body_style
    ))
    
    story.append(Paragraph("• <b>Clignotement ultra-rapide (intervalle de 200ms) :</b> L'appareil est en cours de démarrage, "
                           "d'établissement de la liaison Wi-Fi, ou de négociation avec le courtier MQTT Cloud. Indique une phase transitoire.", bullet_style))
    story.append(Paragraph("• <b>Clignotement lent (intervalle de 500ms) :</b> Le terminal a réussi à s'associer au réseau Wi-Fi local, "
                           "mais il n'a pas encore reçu l'approbation administrative de l'hôpital. Il attend qu'un administrateur lui attribue une chambre "
                           "et un lit dans la base de données.", bullet_style))
    story.append(Paragraph("• <b>Allumée en continu (Fixe H24) :</b> Une alerte est active. Le patient a pressé le bouton, le signal a été "
                           "transmis et persisté. La LED reste allumée jusqu'à ce qu'une infirmière valide/acquitte l'alerte sur son écran.", bullet_style))
    story.append(Paragraph("• Éteinte : L'appareil est en ligne, dûment approuvé par l'administration, et aucun appel n'est actif. "
                           "L'appareil est au repos et consomme le minimum d'énergie.", bullet_style))
    
    # ---------------- PAGE 5: PROTOCOLES DE COMMUNICATION ----------------
    story.append(Paragraph("4. Protocoles de Communication", h1_style))
    story.append(Paragraph(
        "La communication inter-systèmes repose sur un modèle hybride associant légèreté pour l'IoT (MQTT) "
        "et réactivité instantanée pour l'humain (WebSockets WSS). L'Express API HTTP sert de support complémentaire pour "
        "les requêtes synchrones d'administration.",
        body_style
    ))
    
    story.append(Paragraph("4.1 Protocole MQTT (Liaisons IoT)", h2_style))
    story.append(Paragraph(
        "MQTT (Message Queuing Telemetry Transport) est un protocole de messagerie de type publication/abonnement extrêmement "
        "léger, particulièrement adapté aux microcontrôleurs disposant de ressources CPU et réseau limitées.",
        body_style
    ))
    
    story.append(Paragraph("<b>A. Structure des Pings de Santé (Heartbeats) :</b><br/>"
                           "Chaque terminal ESP8266 et le contrôleur central publient périodiquement sur le topic :<br/>"
                           "<font face='Courier' size='8.5'>device/{deviceId}/heartbeat</font> ou <font face='Courier' size='8.5'>controller/{key}/ping</font><br/>"
                           "Le payload JSON transmet la qualité du signal RSSI local et l'uptime matériel.", bullet_style))
    
    story.append(Paragraph("<b>B. Canal d'Alerte :</b><br/>"
                           "Lors de l'appui sur le bouton, le terminal publie instantanément sur le topic :<br/>"
                           "<font face='Courier' size='8.5'>controller/{deviceKey}/alert</font><br/>"
                           "avec un payload JSON identifiant le terminal. Le client MQTT interne au serveur Express intercepte ce message, met à jour "
                           "l'état en base de données et redistribue l'alerte aux soignants.", bullet_style))
    
    story.append(Paragraph("4.2 API REST HTTPS", h2_style))
    story.append(Paragraph(
        "Bien que le temps réel soit géré par MQTT et WebSockets, les opérations administratives et d'administration "
        "courantes s'appuient sur des requêtes HTTP Express standards sécurisées :",
        body_style
    ))
    story.append(Paragraph("• <font face='Courier'>POST /api/devices/approve</font> : Permet d'approuver un terminal en attente et de l'associer à une chambre et un lit.", bullet_style))
    story.append(Paragraph("• <font face='Courier'>POST /api/devices/clear-alert</font> : Déclenché par l'infirmière pour acquitter et éteindre une alerte.", bullet_style))
    story.append(Paragraph("• <font face='Courier'>POST /api/setup/mode</font> : Permet de configurer à distance le mode Wi-Fi du contrôleur central.", bullet_style))
    
    story.append(Paragraph("4.3 Protocole WebSocket (Mise à Jour Instantanée)", h2_style))
    story.append(Paragraph(
        "Afin de rafraîchir le dashboard des infirmiers sans générer de requêtes de polling HTTP incessantes, une connexion "
        "<b>WebSockets sécurisée (WSS)</b> permanente est maintenue entre le client React (Web/Capacitor) et le serveur Node.js. "
        "Les événements transmis sont les suivants :",
        body_style
    ))
    story.append(Paragraph("• <font face='Courier'>FULL_STATE</font> : Transmis immédiatement après la connexion d'un client pour lui envoyer la liste complète des appareils et leur état.", bullet_style))
    story.append(Paragraph("• <font face='Courier'>ALERT</font> : Diffusé dès qu'une alerte est persistée en base de données, déclenchant l'alarme sonore et visuelle sur l'appareil de l'infirmier.", bullet_style))
    story.append(Paragraph("• <font face='Courier'>UPDATE</font> / <font face='Courier'>DELETE</font> : Notifie le dashboard de l'approbation d'un nouveau lit ou du retrait d'un matériel.", bullet_style))
    story.append(Paragraph("• <font face='Courier'>CONTROLLER_STATUS</font> : Permet de remonter la force Wi-Fi (RSSI) et l'état en ligne du contrôleur ESP32-S3 sur le panneau de supervision.", bullet_style))
    
    # ---------------- PAGE 6: BASE DE DONNEES & MONOREPO ----------------
    story.append(Paragraph("5. Modèle de Données & Structure Logicielle", h1_style))
    story.append(Paragraph(
        "La persistance des données repose sur <b>PostgreSQL</b> hébergé sur Railway, requêté via l'ORM TypeScript <b>Drizzle</b> "
        "et validé en amont grâce à des schémas <b>Zod</b> rigoureux.",
        body_style
    ))
    
    story.append(Paragraph("5.1 Schémas des Tables Drizzle", h2_style))
    
    story.append(Paragraph("Table 1 : devices", h3_style))
    devices_db_data = [
        ["Nom Colonne", "Type SQL", "Défaut", "Description / Usage applicatif"],
        ["device_id", "TEXT", "PRIMARY KEY", "Identifiant unique basé sur l'adresse MAC du boîtier (ex: device-2b8f41)."],
        ["patient_name", "TEXT", "''", "Nom complet du patient occupant le lit attribué."],
        ["bed", "TEXT", "''", "Numéro ou libellé du lit dans la chambre."],
        ["room", "TEXT", "''", "Numéro de la chambre ou de l'unité médicale."],
        ["alert_active", "BOOLEAN", "false", "Indique si une alerte d'urgence est active et en cours."],
        ["last_alert_time", "TEXT", "NULL", "Horodatage ISO de la dernière alerte déclenchée."],
        ["registered", "BOOLEAN", "false", "true si l'appareil a contacté le serveur au moins une fois."],
        ["approved", "BOOLEAN", "false", "true si l'administrateur a validé l'appareil et son affectation."],
        ["online", "BOOLEAN", "false", "Indique l'état de connexion actif (heartbeats réguliers)."],
        ["last_seen", "BIGINT", "NULL", "Timestamp Unix (ms) du dernier heartbeat reçu."],
        ["last_updated_at", "BIGINT", "NULL", "Timestamp Unix (ms) de la dernière modification d'état."]
    ]
    story.append(create_styled_table(styles, devices_db_data, [85, 75, 75, 252]))
    story.append(Spacer(1, 10))
    
    story.append(Paragraph("Table 2 : system_settings", h3_style))
    settings_db_data = [
        ["Nom Colonne", "Type SQL", "Défaut", "Description / Usage applicatif"],
        ["id", "INTEGER", "PRIMARY KEY (1)", "Clé primaire forcée à 1 pour assurer une configuration singleton unique."],
        ["controller_last_seen", "BIGINT", "NULL", "Timestamp Unix de la dernière communication de l'ESP32-S3."],
        ["controller_uptime", "INTEGER", "0", "Temps de fonctionnement cumulé de l'ESP32-S3 (en secondes)."],
        ["controller_rssi", "INTEGER", "0", "Force du signal Wi-Fi capté par le contrôleur (en dBm)."],
        ["controller_wifi_error", "TEXT", "NULL", "Dernier message d'erreur réseau renvoyé par l'ESP32-S3."],
        ["wifi_mode", "INTEGER", "1", "Mode Wi-Fi configuré (1: AP, 2: STA, 3: AP+STA, 4: Cloud Sync)."],
        ["pending_command", "TEXT", "NULL", "Commande en attente de synchronisation pour le contrôleur."],
        ["command_params", "TEXT", "NULL", "Paramètres JSON additionnels pour la commande en attente."]
    ]
    story.append(create_styled_table(styles, settings_db_data, [100, 75, 80, 232]))
    
    story.append(Spacer(1, 10))
    
    # Monorepo structure page
    story.append(Paragraph("5.2 Structure du Projet (Monorepo)", h2_style))
    story.append(Paragraph(
        "Le projet est organisé sous forme de monorepo TypeScript propre, clarifiant la séparation entre "
        "le micrologiciel embarqué C++ et la logique d'application Web et Mobile :",
        body_style
    ))
    
    tree_text = """My-PFC/
|-- server/               # Backend Node.js / Express / WebSockets & Aedes MQTT
|   |-- index.ts          # Point d'entrée de l'application & initialisation de la base
|   |-- routes.ts         # Définition des endpoints REST HTTP & Middleware
|   |-- wss.ts            # Gestion du serveur de WebSockets
|   |-- mqtt.ts           # Logique du broker MQTT intégré (Aedes) & gestion des queues
|   |-- storage.ts        # Logique d'accès aux données (PostgreSQL via Drizzle ORM)
|   |-- db.ts             # Configuration du pool de connexion PostgreSQL
|   \\-- static.ts         # Serveur de fichiers statiques pour le SPA React
|-- shared/               # Code partagé entre le frontend et le backend
|   \\-- schema.ts         # Modèles de base de données Drizzle et schémas de validation Zod
|-- client/               # Application Frontend React (Servie par Node.js)
|   |-- src/
|   |   |-- App.tsx       # Routage (wouter) et initialisation
|   |   |-- components/   # Composants réutilisables (DeviceCard, SetupWizard)
|   |   \\-- hooks/        # Hooks personnalisés (useAlertSound, use-mobile)
|   \\-- vite.config.ts    # Config de build frontend React
|-- android/              # Application Mobile (Capacitor WebView)
|   \\-- capacitor.config.ts # Pointe l'application mobile vers https://my-pfc-production.up.railway.app
\\-- esp32/                # Firmware des microcontrôleurs (C++)
    |-- controller/       # Code du contrôleur central ESP32-S3 (Buzzer, Passerelle)
    \\-- device/           # Code du boîtier patient ESP8266 (Bouton, LED)"""
    
    story.append(create_code_block(styles, tree_text))
    story.append(Spacer(1, 10))
    
    # ---------------- PAGE 7: SECURITE ET RESILIENCE ----------------
    story.append(Paragraph("6. Sécurité, Avantages & Résilience", h1_style))
    story.append(Paragraph(
        "La conception technique de la nouvelle architecture monolithique centralisée offre des garanties robustes "
        "en termes de résilience matérielle, de réactivité réseau et de facilité de déploiement en production.",
        body_style
    ))
    
    story.append(Paragraph("6.1 Avantages de l'Intégration Mobile via Capacitor", h2_style))
    story.append(Paragraph(
        "L'adoption de <b>Capacitor WebView</b> en remplacement d'une architecture native compilée (type React Native classique) "
        "présente des bénéfices considérables pour l'exploitation hospitalière :",
        body_style
    ))
    story.append(Paragraph("• <b>Mises à jour instantanées (Over-The-Air) :</b> Toute correction de bug ou amélioration du tableau de bord "
                           "effectuée sur le dépôt git et déployée sur Railway est instantanément visible sur le smartphone des infirmiers. "
                           "Il n'est pas nécessaire de reconstruire le fichier APK, ni de forcer les soignants à réinstaller manuellement l'application.", bullet_style))
    story.append(Paragraph("• <b>Coquille native optimisée :</b> Capacitor permet de conserver l'accès aux vibrations physiques, aux "
                           "notifications push système et à l'état de la batterie du smartphone, tout en bénéficiant de la simplicité et de "
                           "la rapidité d'exécution du HTML5 / React moderne.", bullet_style))
    
    story.append(Paragraph("6.2 Robustesse face aux limitations Serverless", h2_style))
    story.append(Paragraph(
        "Initialement hébergé sur Vercel, le backend rencontrait des limites inhérentes aux fonctions Serverless : "
        "déconnexions incessantes des WebSockets dues aux temps d'exécution maximum (timeouts de 10 à 60 secondes) et "
        "incapacité de faire tourner un broker MQTT persistant en arrière-plan. "
        "L'hébergement sous forme de <b>Monolithe Railway (Dockerisé)</b> garantit un processus Node.js persistant H24. "
        "La latence d'acheminement d'une alerte est inférieure à 100ms, et la charge CPU du serveur reste minime.",
        body_style
    ))
    
    story.append(Paragraph("6.3 File d'Attente de Commandes (Resilience Mode)", h2_style))
    story.append(Paragraph(
        "Le module de communication MQTT du serveur (<font face='Courier'>server/mqtt.ts</font>) intègre une gestion intelligente "
        "des files d'attente hors-ligne. Si l'ESP32-S3 perd momentanément sa connexion Wi-Fi à cause d'une perturbation dans l'hôpital, "
        "les requêtes vitales (comme l'acquittement ou l'arrêt de la sirène) ne sont pas perdues. Elles sont empilées dans une queue "
        "en mémoire et re-émises immédiatement dès la détection de la reconnexion de la passerelle.",
        body_style
    ))
    
    story.append(Paragraph("6.4 Modèle de Sécurité Hybride", h2_style))
    story.append(Paragraph(
        "Les données transitant dans le réseau hospitalier sont protégées par une authentification à deux niveaux :",
        body_style
    ))
    story.append(Paragraph("1. <b>Sécurité IoT :</b> Les microcontrôleurs (ESP32 / ESP8266) s'authentifient auprès du broker MQTT cloud "
                           "via des identifiants sécurisés et injectent un token d'appareil unique (<font face='Courier'>X-Device-Key</font>) "
                           "dans l'en-tête de toutes les communications REST HTTP, évitant l'usurpation de lit.", bullet_style))
    story.append(Paragraph("2. <b>Sécurité Staff Médical :</b> L'accès au panneau de configuration et de suivi web par les infirmières "
                           "et médecins est restreint par un mécanisme d'authentification classique (Passport.js) reposant sur des sessions "
                           "chiffrées par cookie sécurisé.", bullet_style))
    
    # Build Document
    print(f"Generating professional PDF report: {filename}...")
    doc.build(story, canvasmaker=NumberedCanvas)
    print("PDF Generation complete.")

if __name__ == "__main__":
    pdf_path = r"c:\Users\zined\Documents\GitHub\My-PFC\PFC_TECHNICAL_DOCUMENTATION.pdf"
    build_pdf(pdf_path)
