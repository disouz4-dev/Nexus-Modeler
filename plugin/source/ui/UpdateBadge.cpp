#include "UpdateBadge.h"

#include "Language.h"
#include "Theme.h"

namespace nexus::ui
{
using nexus::update::State;

namespace
{
juce::String tr(const char* text)
{
  return TRANS(juce::String::fromUTF8(text));
}
} // namespace

UpdateBadge::UpdateBadge()
{
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  mUpdater->addChangeListener(this);
  mUpdater->checkIfDue();
  refresh();
}

UpdateBadge::~UpdateBadge()
{
  mUpdater->removeChangeListener(this);
}

juce::String UpdateBadge::text() const
{
  const auto s = mUpdater->state();
  const auto version = mUpdater->release().version;
  if (s == State::downloading)
    return tr("Baixando a versão %1").replace("%1", version) + "  " + juce::String(juce::roundToInt(mUpdater->progress() * 100.0f)) + "%";
  if (s == State::ready)
    return mUpdater->installedInPlace() ? tr("Versão %1 instalada: reiniciar").replace("%1", version) : tr("Instalador aberto");
  if (s == State::failed)
    return tr("Falha ao baixar a versão %1").replace("%1", version);
  return tr("Nova versão %1").replace("%1", version);
}

int UpdateBadge::idealWidth() const
{
  return juce::GlyphArrangement::getStringWidthInt(Fonts::get().ui(12.5f), text()) + 44;
}

void UpdateBadge::refresh()
{
  const bool show = mUpdater->showBadge();
  if (show != isVisible())
    setVisible(show);
  const auto s = mUpdater->state();
  if (s == State::ready && mUpdater->installedInPlace())
    setTooltip(tr("Clique para reabrir o Nexus já na versão nova"));
  else if (s == State::ready)
    setTooltip(tr("O instalador foi aberto. Feche o Nexus e o programa de música onde ele está aberto para terminar."));
  else if (s == State::failed)
    setTooltip(mUpdater->error());
  else
    setTooltip(tr("Clique para baixar e instalar a versão nova"));
  if (onLayoutChange)
    onLayoutChange();
  repaint();
}

void UpdateBadge::changeListenerCallback(juce::ChangeBroadcaster*)
{
  const auto s = mUpdater->state();
  const bool finished = s == State::ready && mShown != State::ready;
  mShown = s;
  refresh();
  if (finished)
  {
    const auto version = mUpdater->release().version;
    if (mUpdater->installedInPlace())
    {
      // Linux pela pasta do usuário: já trocou os arquivos; só falta reabrir.
      if (juce::JUCEApplicationBase::isStandaloneApp())
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::NoIcon, tr("Versão %1 instalada").replace("%1", version),
                                           tr("Reabra o Nexus para usar a versão nova."), tr("Reabrir agora"), tr("Depois"), nullptr,
                                           juce::ModalCallbackFunction::create([](int ok) {
                                             if (ok == 1)
                                               nexus::update::Updater::restartProgram();
                                           }));
      else
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, tr("Versão %1 instalada").replace("%1", version),
                                               tr("Feche e abra de novo o programa de música para usar a versão nova."));
    }
    else
    {
#if JUCE_WINDOWS
      const auto message = tr("O instalador está atualizando. O Nexus fecha e abre sozinho na versão nova; no programa de música, feche e abra de novo.");
#else
      const auto message = tr("O instalador foi aberto (o arquivo está na pasta Downloads). Para terminar, feche o Nexus e o programa de música onde ele está aberto e siga o instalador.");
#endif
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, tr("Versão %1 baixada").replace("%1", version),
                                             message + "\n\n" + mUpdater->downloadedFile().getFullPathName());
    }
  }
  if (mUpdater->takeManualResult())
    showResult();
}

// Resultado de "Procurar atualizações agora".
void UpdateBadge::showResult()
{
  const auto s = mUpdater->state();
  if (s == State::available)
  {
    showMenu();
    return;
  }
  const auto message = s == State::upToDate
                         ? tr("Você já está na versão mais nova (%1).").replace("%1", nexus::update::currentVersion())
                         : tr("Não foi possível procurar atualizações agora.") + "\n" + mUpdater->error();
  juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, tr("Atualizações"), message);
}

void UpdateBadge::showMenu()
{
  const auto s = mUpdater->state();
  if (s == State::downloading)
    return;
  juce::PopupMenu menu;
  const auto version = mUpdater->release().version;
  menu.addSectionHeader(tr("Versão %1 (você tem a %2)").replace("%1", version).replace("%2", nexus::update::currentVersion()));
  if (s == State::ready && mUpdater->installedInPlace())
    menu.addItem(7, tr("Reabrir o Nexus agora"), juce::JUCEApplicationBase::isStandaloneApp());
  else if (s == State::ready)
  {
    menu.addItem(1, tr("Abrir o instalador de novo"));
    menu.addItem(2, tr("Mostrar na pasta"));
  }
  else
  {
    const bool installer = mUpdater->release().assetUrl.isNotEmpty();
    menu.addItem(3, s == State::failed ? tr("Tentar baixar de novo")
                    : installer       ? tr("Baixar e instalar agora")
                                      : tr("Baixar na página do lançamento"));
  }
  menu.addItem(4, tr("Ver o que mudou"));
  menu.addSeparator();
  menu.addItem(5, tr("Lembrar depois"));
  menu.addItem(6, tr("Ignorar esta versão"));
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this](int r) {
    if (r == 1)
      mUpdater->downloadedFile().startAsProcess();
    else if (r == 2)
      mUpdater->downloadedFile().revealToUser();
    else if (r == 3)
      mUpdater->downloadAndOpen();
    else if (r == 4)
      mUpdater->openPage();
    else if (r == 5)
      mUpdater->dismiss();
    else if (r == 6)
      mUpdater->skipThisVersion();
    else if (r == 7)
      nexus::update::Updater::restartProgram();
  });
}

void UpdateBadge::addMenuItems(juce::PopupMenu& menu)
{
  menu.addItem(tr("Procurar atualizações agora"), [this] { mUpdater->checkNow(); });
  menu.addItem(tr("Procurar atualizações sozinho ao abrir"), true, mUpdater->automatic(),
               [this] { mUpdater->setAutomatic(!mUpdater->automatic()); });
}

void UpdateBadge::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat().reduced(1.0f, 6.0f);
  const bool failed = mUpdater->state() == State::failed;
  const auto tint = failed ? colour::danger : colour::accent;
  g.setColour(tint.withAlpha(isMouseOver(true) ? 0.22f : 0.14f));
  g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
  g.setColour(tint.withAlpha(0.6f));
  g.drawRoundedRectangle(r.reduced(0.5f), r.getHeight() * 0.5f, 1.0f);

  // Barra de progresso dentro da pílula durante o download.
  if (mUpdater->state() == State::downloading)
  {
    auto bar = r.reduced(r.getHeight() * 0.5f, 0.0f).removeFromBottom(2.0f).translated(0.0f, -4.0f);
    g.setColour(colour::accent.withAlpha(0.25f));
    g.fillRect(bar);
    g.setColour(colour::accent);
    g.fillRect(bar.withWidth(bar.getWidth() * juce::jlimit(0.0f, 1.0f, mUpdater->progress())));
  }

  // Ponto aceso à esquerda, como o LED de um equipamento.
  const float d = 7.0f;
  g.setColour(tint);
  g.fillEllipse(r.getX() + 13.0f, r.getCentreY() - d * 0.5f, d, d);
  g.setColour(colour::text);
  g.setFont(Fonts::get().ui(12.5f));
  g.drawText(text(), r.withTrimmedLeft(28.0f).withTrimmedRight(12.0f), juce::Justification::centredLeft, true);
}

void UpdateBadge::mouseUp(const juce::MouseEvent& e)
{
  if (contains(e.getPosition()))
    showMenu();
}
} // namespace nexus::ui
