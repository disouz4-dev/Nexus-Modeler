#include "CapturePanel.h"

#include "Language.h"
#include "Theme.h"
#include "../report/Report.h"

namespace nexus::ui
{
namespace
{
constexpr int kRowHeight = 58;

// Marca da TONE3000 em texto (a fonte das capturas fica sempre à vista).
void drawSource(juce::Graphics& g, juce::Rectangle<float> r)
{
  g.setColour(colour::dim);
  g.setFont(Fonts::get().ui(11.0f));
  const auto label = TRANS("Fonte:") + " ";
  const float lw = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), label));
  const auto brand = Fonts::get().display(16.0f).withExtraKerningFactor(0.06f);
  const float bw = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(brand, "TONE3000"));
  auto line = r.withTrimmedLeft(r.getWidth() - lw - bw);
  g.drawText(label, line.removeFromLeft(lw), juce::Justification::centredRight);
  g.setColour(colour::text);
  g.setFont(brand);
  g.drawText("TONE3000", line, juce::Justification::centredLeft);
}

juce::String thousands(int n)
{
  if (n >= 1000)
    return juce::String(n / 1000.0, 1).replaceCharacter('.', ',') + " mil";
  return juce::String(n);
}
} // namespace

CapturePanel::CapturePanel()
{
  addAndMakeVisible(mTabs);
  mTabs.setFontSize(11.5f);
  mTabs.onSelect = [this](int i) { showTab(i); };
  addAndMakeVisible(mList);
  mList.setRowHeight(kRowHeight);
  mList.setMouseMoveSelectsRows(true); // a linha sob o mouse acende
  mList.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
  mList.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
  mList.getViewport()->setScrollBarThickness(8);
  for (auto* b : {&mSignIn, &mCatalog, &mManual, &mClose})
    addAndMakeVisible(*b);
  addAndMakeVisible(mSignOut);
  mSignIn.onClick = [this] {
    setStatus(TRANS(juce::String::fromUTF8("Abrindo a TONE3000 no navegador...")));
    mConnection->signIn([self = juce::Component::SafePointer<CapturePanel>(this)](juce::String error) {
      if (error.isNotEmpty())
        nexus::report::log("tone3000", "conectar: " + error);
      if (self == nullptr)
        return;
      self->setStatus(error.isEmpty() ? TRANS("Conta da TONE3000 conectada.") : error, error.isNotEmpty());
      self->refreshAccount();
      self->showTab(self->mTab);
    });
  };
  mSignOut.onClick = [this] {
    mConnection->signOut();
    refreshAccount();
    showTab(mTab);
  };
  mCatalog.onClick = [this] { load({}); };
  mManual.onClick = [this] {
    if (onManual)
      onManual();
  };
  mClose.onClick = [this] {
    if (onClose)
      onClose();
  };
}

CapturePanel::~CapturePanel()
{
  mList.setModel(nullptr);
}

void CapturePanel::open(const juce::String& gearPrefix, const juce::String& name, Gear gear)
{
  mPrefix = gearPrefix;
  mName = name;
  mGear = gear;
  mBusy = false;
  setStatus({});
  retranslate();
  refreshAccount();
  // Conectado: abre nas mais curtidas; sem conta, nas recomendadas.
  const int first = mConnection->hasKey() && mConnection->signedIn() ? 0 : 1;
  mTabs.setSelected(first);
  showTab(first);
}

void CapturePanel::retranslate()
{
  mTabs.setItems({TRANS("Mais curtidas"), TRANS("Recomendadas"), TRANS("Seus favoritos"), TRANS("Seus downloads"), TRANS("Em alta")});
  mSignIn.setButtonText(TRANS("Entrar com a TONE3000"));
  mSignOut.setText(TRANS("sair"));
  mCatalog.setButtonText(TRANS(juce::String::fromUTF8("Catálogo completo da TONE3000")));
  mManual.setButtonText(TRANS("Inserir manualmente"));
  mClose.setButtonText(TRANS("Fechar"));
  repaint();
}

void CapturePanel::refreshAccount()
{
  const bool key = mConnection->hasKey();
  const bool in = key && mConnection->signedIn();
  mSignIn.setVisible(key && !in);
  mSignOut.setVisible(in);
  mCatalog.setEnabled(key && !mBusy);
  mTabs.setItemEnabled(0, key);
  mTabs.setItemEnabled(2, key);
  mTabs.setItemEnabled(3, key);
  mTabs.setItemEnabled(4, key);
  resized();
  repaint();
}

void CapturePanel::setStatus(const juce::String& text, bool error)
{
  mStatus = text;
  mStatusError = error;
  repaint();
}

void CapturePanel::showTab(int tab)
{
  mTab = tab;
  mTones.clear();
  mEmpty.clear();
  const int request = ++mRequest;
  const auto word = t3k::filterWord(mPrefix);
  // Abas: 0 mais curtidas (busca ordenada por favoritos), 1 recomendadas,
  // 2 seus favoritos, 3 seus downloads, 4 em alta.
  if (tab == 1)
  {
    mTones = t3k::recommended(mPrefix);
    if (mTones.empty())
      mEmpty = TRANS(juce::String::fromUTF8("Ainda não há recomendadas para este equipamento. Use o catálogo completo."));
  }
  else if (!mConnection->hasKey())
    mEmpty = TRANS(juce::String::fromUTF8("Esta versão do Nexus foi compilada sem a chave da TONE3000."));
  else if (tab != 4 && !mConnection->signedIn())
    mEmpty = TRANS("Entre com a sua conta da TONE3000 para ver seus favoritos e downloads.");
  else
  {
    mEmpty = TRANS("Carregando...");
    const auto which = tab == 0   ? t3k::Connection::List::mostLiked
                       : tab == 2 ? t3k::Connection::List::favorites
                       : tab == 3 ? t3k::Connection::List::downloaded
                                  : t3k::Connection::List::trending;
    auto show = [self = juce::Component::SafePointer<CapturePanel>(this), request, tab, word](std::vector<t3k::Tone> tones, juce::String error) {
      if (self == nullptr || self->mRequest != request)
        return;
      self->mTones = std::move(tones);
      if (error.isNotEmpty())
      {
        nexus::report::log("tone3000", "lista: " + error);
        self->mEmpty = error;
      }
      else if (self->mTones.empty())
        self->mEmpty = tab == 4   ? TRANS(juce::String::fromUTF8("Nada em alta para este tipo agora."))
                       : tab == 0 ? TRANS(juce::String::fromUTF8("Nenhum tom de %1 com captura A2 na busca. Veja as recomendadas.")).replace("%1", word)
                                  : TRANS(juce::String::fromUTF8("Nenhum tom de %1 aqui. Marque favoritos no catálogo da TONE3000.")).replace("%1", word);
      self->mList.updateContent();
      self->repaint();
    };
    // A busca separa os tipos com "_": cabeçote é amp ou amp com caixa.
    const auto gear = tab == 0 && gearType() == "amp" ? juce::String("amp_amp-cab") : gearType();
    mConnection->list(which, gear, tab == 4 ? juce::String() : word, show);
  }
  mList.updateContent();
  mList.scrollToEnsureRowIsOnscreen(0);
  repaint();
}

void CapturePanel::load(const t3k::Tone& tone)
{
  if (mBusy)
    return;
  if (!mConnection->hasKey())
  {
    setStatus(TRANS(juce::String::fromUTF8("Esta versão do Nexus foi compilada sem a chave da TONE3000.")), true);
    return;
  }
  mBusy = true;
  refreshAccount();
  auto self = juce::Component::SafePointer<CapturePanel>(this);
  mConnection->download(
    tone, mGear == Gear::pedal ? "pedal" : mGear == Gear::cab ? "cab" : "amp_amp-cab", mName,
    [self](juce::String text) {
      if (self != nullptr)
        self->setStatus(text);
    },
    [self](juce::File folder, juce::String error) {
      if (self == nullptr)
        return;
      self->mBusy = false;
      self->refreshAccount();
      if (error.isNotEmpty())
      {
        nexus::report::log("tone3000", "baixar: " + error);
        self->setStatus(error, true);
        return;
      }
      self->setStatus({});
      if (self->onLoaded)
        self->onLoaded(folder);
    });
}

void CapturePanel::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool hover)
{
  if (row < 0 || row >= static_cast<int>(mTones.size()))
    return;
  const auto& t = mTones[static_cast<size_t>(row)];
  auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.0f, 4.0f);
  g.setColour(juce::Colour(hover ? 0xff202228 : 0xff131417));
  g.fillRoundedRectangle(r, 8.0f);
  auto inner = r.reduced(16.0f, 8.0f);

  // Botão "Carregar" desenhado na linha (a linha toda é clicável).
  auto button = inner.removeFromRight(110.0f).withSizeKeepingCentre(110.0f, 30.0f);
  g.setColour(hover && !mBusy ? colour::accent : colour::boxBorder);
  g.drawRoundedRectangle(button.reduced(0.5f), 6.0f, 1.0f);
  drawCaps(g, TRANS("Carregar"), button, 10.5f, juce::Justification::centred, mBusy ? colour::faint : (hover ? colour::accent : colour::text));

  inner.removeFromRight(12.0f);
  g.setColour(colour::text);
  g.setFont(Fonts::get().ui(14.0f));
  g.drawText(t.packName.isNotEmpty() ? t.title + juce::String::fromUTF8("  ·  ") + TRANS(t.packName) : t.title,
             inner.removeFromTop(inner.getHeight() * 0.5f), juce::Justification::centredLeft, true);
  juce::StringArray details;
  if (t.creator.isNotEmpty())
    details.add("@" + t.creator);
  if (t.models > 0)
    details.add((mGear == Gear::cab ? TRANS("%1 IRs") : TRANS("%1 capturas A2")).replace("%1", juce::String(t.models)));
  if (t.note.isNotEmpty())
    details.add(TRANS(t.note));
  if (t.favorites > 0)
    details.add(TRANS("%1 favoritos").replace("%1", thousands(t.favorites)));
  if (t.downloads > 0)
    details.add(TRANS("%1 downloads").replace("%1", thousands(t.downloads)));
  g.setColour(colour::dim);
  g.setFont(Fonts::get().ui(12.0f));
  g.drawText(details.joinIntoString(juce::String::fromUTF8("  ·  ")), inner, juce::Justification::centredLeft, true);
}

void CapturePanel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
  if (row >= 0 && row < static_cast<int>(mTones.size()))
    load(mTones[static_cast<size_t>(row)]);
}

juce::Rectangle<int> CapturePanel::card() const
{
  return getLocalBounds().withSizeKeepingCentre(std::min(820, getWidth() - 40), std::min(600, getHeight() - 40));
}

void CapturePanel::resized()
{
  auto r = card().reduced(32, 0);
  r.removeFromTop(78);
  mTabs.setBounds(r.removeFromTop(34).withWidth(std::min(r.getWidth(), 560)));
  r.removeFromTop(12);
  auto bottom = r.removeFromBottom(76);
  r.removeFromBottom(30); // linha de situação
  mList.setBounds(r);
  auto buttons = bottom.withTrimmedTop(20).withTrimmedBottom(20);
  mClose.setBounds(buttons.removeFromRight(100));
  buttons.removeFromRight(10);
  mManual.setBounds(buttons.removeFromRight(170));
  buttons.removeFromRight(10);
  mCatalog.setBounds(buttons.removeFromRight(240));
  mSignIn.setBounds(buttons.removeFromLeft(200));
  // "sair" fica ao lado do nome da conta, desenhado em paint().
  const auto font = Fonts::get().ui(12.0f);
  const auto who = TRANS("Conectado como @%1").replace("%1", mConnection->userName());
  const int whoW = juce::GlyphArrangement::getStringWidthInt(font, who);
  mSignOut.setBounds(buttons.getX() + whoW + 10, buttons.getY(), mSignOut.idealWidth(), buttons.getHeight());
}

void CapturePanel::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colours::black.withAlpha(0.6f));
  const auto c = card().toFloat();
  g.setColour(juce::Colour(0xff17181b));
  g.fillRoundedRectangle(c, 12.0f);
  g.setColour(colour::boxBorder);
  g.drawRoundedRectangle(c.reduced(0.5f), 12.0f, 1.0f);

  auto r = c.reduced(32.0f, 0.0f);
  auto head = r.removeFromTop(78.0f);
  drawSource(g, head.withTrimmedTop(14.0f).removeFromTop(30.0f));
  g.setColour(colour::text);
  g.setFont(Fonts::get().display(20.0f).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.14f : 0.0f));
  g.drawText(upper((mGear == Gear::cab ? TRANS("IRs para %1") : TRANS("Capturas para %1")).replace("%1", mName)), head.withTrimmedTop(14.0f).removeFromTop(30.0f).withTrimmedRight(200.0f),
             juce::Justification::centredLeft);
  g.setColour(colour::accent);
  g.fillRect(head.getX(), head.getBottom() - 22.0f, 28.0f, 2.0f);

  if (mTones.empty() && mEmpty.isNotEmpty())
  {
    g.setColour(colour::dim);
    g.setFont(Fonts::get().ui(13.0f));
    g.drawFittedText(mEmpty, mList.getBounds().reduced(20), juce::Justification::centred, 3);
  }

  // Situação: o que está sendo baixado, ou o erro.
  auto status = juce::Rectangle<float>(r.getX(), static_cast<float>(mList.getBottom()) + 6.0f, r.getWidth(), 22.0f);
  if (mStatus.isNotEmpty())
  {
    g.setColour(mStatusError ? colour::danger : colour::accent);
    g.setFont(Fonts::get().ui(12.5f));
    g.drawFittedText(mStatus, status.toNearestInt(), juce::Justification::centredLeft, 1);
  }
  else if (mTab == 0 && !mTones.empty())
  {
    g.setColour(colour::faint);
    g.setFont(Fonts::get().ui(11.5f));
    g.drawFittedText(mGear == Gear::cab
                       ? TRANS(juce::String::fromUTF8("Escolhidas pelo Nexus: cada pacote traz vários microfones; troque na própria caixa. Os IRs são dos criadores e baixados da TONE3000."))
                       : TRANS(juce::String::fromUTF8("Escolhidas pelo Nexus: uma captura para cada posição do botão. As capturas são dos criadores e baixadas da TONE3000.")),
                     status.toNearestInt(), juce::Justification::centredLeft, 1);
  }

  if (mSignOut.isVisible())
  {
    g.setColour(colour::dim);
    g.setFont(Fonts::get().ui(12.0f));
    g.drawText(TRANS("Conectado como @%1").replace("%1", mConnection->userName()),
               juce::Rectangle<int>(mSignIn.getX(), mSignOut.getY(), 400, mSignOut.getHeight()), juce::Justification::centredLeft);
  }
  else if (!mConnection->hasKey())
  {
    g.setColour(colour::faint);
    g.setFont(Fonts::get().ui(12.0f));
    g.drawText(TRANS(juce::String::fromUTF8("TONE3000 desligada nesta versão")), juce::Rectangle<int>(mSignIn.getX(), mSignIn.getY(), 300, mSignIn.getHeight()),
               juce::Justification::centredLeft);
  }
}

void CapturePanel::mouseUp(const juce::MouseEvent& e)
{
  if (!card().contains(e.getPosition()) && onClose)
    onClose();
}
} // namespace nexus::ui
