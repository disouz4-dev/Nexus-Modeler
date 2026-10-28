#include "UpdateBadge.h"

#include "Controls.h"
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

void UpdateBadge::refresh()
{
  const bool show = mUpdater->showBadge();
  if (show != isVisible())
    setVisible(show);
  const auto s = mUpdater->state();
  juce::String hint;
  if (s == State::ready && mUpdater->installedInPlace())
    hint = tr("Clique para reabrir o Nexus já na versão nova");
  else if (s == State::ready)
    hint = tr("O instalador foi aberto. Feche o Nexus e o programa de música onde ele está aberto para terminar.");
  else if (s == State::failed)
    hint = mUpdater->error();
  else if (s != State::downloading)
    hint = tr("Clique para atualizar");
  setTooltip(hint.isEmpty() ? text() : text() + "\n" + hint);
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
  // Ao abrir o Nexus: versão nova encontrada, pergunta uma vez.
  else if (s == State::available && isVisible() && mUpdater->takeFirstAsk())
    ask();
}

// "Quer atualizar?" com duas respostas. Agora não: o ícone continua no topo.
void UpdateBadge::ask()
{
  const auto s = mUpdater->state();
  if (s != State::available && s != State::failed)
  {
    showMenu();
    return;
  }
  mUpdater->takeFirstAsk(); // já perguntou nesta abertura
  const auto version = mUpdater->release().version;
  const bool installer = mUpdater->release().assetUrl.isNotEmpty();
  auto message = tr("Você está na versão %1. Quer atualizar agora?").replace("%1", nexus::update::currentVersion());
  if (s == State::failed)
    message = tr("Não foi possível baixar a versão %1.").replace("%1", version) + "\n" + mUpdater->error() + "\n\n"
              + tr("Quer tentar de novo?");
  juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::NoIcon, tr("Nova versão %1 disponível").replace("%1", version), message,
                                     installer ? tr("Atualizar agora") : tr("Abrir a página"), tr("Agora não"), this,
                                     juce::ModalCallbackFunction::create([this, installer](int ok) {
                                       if (ok != 1)
                                         return;
                                       if (installer)
                                         mUpdater->downloadAndOpen();
                                       else
                                         mUpdater->openPage();
                                     }));
}

// Resultado de "Procurar atualizações agora".
void UpdateBadge::showResult()
{
  const auto s = mUpdater->state();
  if (s == State::available)
  {
    ask();
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
  auto r = getLocalBounds().toFloat();
  const auto s = mUpdater->state();
  const auto tint = s == State::failed ? colour::danger : colour::accent;
  g.setColour(tint.withAlpha(isMouseOver(true) ? 0.24f : 0.14f));
  g.fillEllipse(r.reduced(4.0f));

  // Anel do download em volta do ícone.
  if (s == State::downloading)
  {
    const auto ring = r.reduced(3.0f);
    juce::Path arc;
    arc.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f, ring.getHeight() * 0.5f, 0.0f, 0.0f,
                      juce::MathConstants<float>::twoPi * juce::jlimit(0.0f, 1.0f, mUpdater->progress()), true);
    g.setColour(tint.withAlpha(0.25f));
    g.drawEllipse(ring, 2.0f);
    g.setColour(tint);
    g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  }

  g.setColour(tint);
  g.fillPath(iconPath(Icon::download, r.withSizeKeepingCentre(20.0f, 20.0f)));

  // Ponto aceso no canto: tem algo para fazer.
  if (s == State::available || s == State::failed || s == State::ready)
  {
    const float d = 8.0f;
    const auto dot = juce::Rectangle<float>(d, d).withPosition(r.getRight() - d - 4.0f, r.getY() + 4.0f);
    g.setColour(colour::danger);
    g.fillEllipse(dot);
  }
}

void UpdateBadge::mouseUp(const juce::MouseEvent& e)
{
  if (!contains(e.getPosition()))
    return;
  if (e.mods.isPopupMenu())
    showMenu();
  else
    ask();
}
} // namespace nexus::ui
