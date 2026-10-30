#include "PresetBar.h"

#include "Language.h"
#include "Theme.h"
#include "../update/NovaInstalacao.h"

namespace nexus::ui
{
PresetBar::PresetBar(std::function<juce::MemoryBlock()> getState, std::function<void(const juce::MemoryBlock&)> setState)
: mGetState(std::move(getState))
, mSetState(std::move(setState))
{
  addAndMakeVisible(mPrev);
  addAndMakeVisible(mNext);
  mPrev.onClick = [this] { step(-1); };
  mNext.onClick = [this] { step(1); };
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

juce::File PresetBar::folder()
{
  return nexus::update::dataRoot().getChildFile("Presets");
}

juce::Array<juce::File> PresetBar::list() const
{
  auto files = folder().findChildFiles(juce::File::findFiles, false, "*.nexus");
  files.sort();
  return files;
}

void PresetBar::load(const juce::File& file)
{
  juce::MemoryBlock data;
  if (!file.loadFileAsData(data) || data.isEmpty())
    return;
  mCurrent = file;
  mSetState(data);
  repaint();
}

void PresetBar::step(int delta)
{
  const auto files = list();
  if (files.isEmpty())
    return;
  int i = files.indexOf(mCurrent);
  i = i < 0 ? (delta > 0 ? 0 : files.size() - 1) : (i + delta + files.size()) % files.size();
  load(files[i]);
}

void PresetBar::askName(const juce::String& title, const juce::String& initial, std::function<void(juce::String)> done)
{
  mDialog = std::make_unique<juce::AlertWindow>(title, TRANS("Nome do preset"), juce::MessageBoxIconType::NoIcon, this);
  mDialog->addTextEditor("nome", initial);
  mDialog->addButton(TRANS("Salvar"), 1, juce::KeyPress(juce::KeyPress::returnKey));
  mDialog->addButton(TRANS("Cancelar"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
  mDialog->enterModalState(true, juce::ModalCallbackFunction::create([this, done](int result) {
                             const auto name = mDialog ? mDialog->getTextEditorContents("nome").trim() : juce::String();
                             mDialog.reset();
                             if (result == 1 && name.isNotEmpty())
                               done(juce::File::createLegalFileName(name));
                           }),
                           false);
}

void PresetBar::save(bool ask)
{
  auto write = [this](const juce::File& file) {
    folder().createDirectory();
    const auto data = mGetState();
    if (file.replaceWithData(data.getData(), data.getSize()))
      mCurrent = file;
    repaint();
  };
  if (!ask && mCurrent.existsAsFile())
  {
    write(mCurrent);
    return;
  }
  askName(TRANS("Salvar preset"), currentName(), [this, write](juce::String name) { write(folder().getChildFile(name + ".nexus")); });
}

void PresetBar::showMenu()
{
  juce::PopupMenu menu;
  const auto files = list();
  menu.addSectionHeader(TRANS("Presets"));
  for (int i = 0; i < files.size(); ++i)
    menu.addItem(100 + i, files[i].getFileNameWithoutExtension(), true, files[i] == mCurrent);
  if (files.isEmpty())
    menu.addItem(-1, TRANS("Nenhum preset salvo ainda"), false);
  menu.addSeparator();
  menu.addItem(1, TRANS("Salvar"));
  menu.addItem(2, TRANS("Salvar como..."));
  menu.addItem(3, TRANS("Renomear..."), mCurrent.existsAsFile());
  menu.addItem(4, TRANS("Apagar este preset"), mCurrent.existsAsFile());
  menu.addItem(5, TRANS("Abrir a pasta dos presets"));
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, files](int r) {
    if (r >= 100 && r - 100 < files.size())
      load(files[r - 100]);
    else if (r == 1)
      save(false);
    else if (r == 2)
      save(true);
    else if (r == 3)
      askName(TRANS("Renomear preset"), currentName(), [this](juce::String name) {
        const auto target = folder().getChildFile(name + ".nexus");
        if (mCurrent.moveFileTo(target))
          mCurrent = target;
        repaint();
      });
    else if (r == 4)
    {
      // Vai para a lixeira: dá para recuperar.
      mCurrent.moveToTrash();
      mCurrent = juce::File();
      repaint();
    }
    else if (r == 5)
    {
      folder().createDirectory();
      folder().revealToUser();
    }
  });
}

juce::Rectangle<int> PresetBar::nameArea() const
{
  return getLocalBounds().reduced(36, 0);
}

void PresetBar::resized()
{
  auto r = getLocalBounds();
  mPrev.setBounds(r.removeFromLeft(32).withSizeKeepingCentre(28, 28));
  mNext.setBounds(r.removeFromRight(32).withSizeKeepingCentre(28, 28));
}

void PresetBar::paint(juce::Graphics& g)
{
  auto r = nameArea().toFloat().reduced(4.0f, 4.0f);
  g.setColour(juce::Colour(isMouseOver(true) ? 0xff1c1d21 : 0xff141518));
  g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
  g.setColour(colour::boxBorder);
  g.drawRoundedRectangle(r.reduced(0.5f), r.getHeight() * 0.5f, 1.0f);
  const auto name = currentName();
  g.setColour(name.isEmpty() ? colour::dim : colour::text);
  g.setFont(Fonts::get().ui(13.0f));
  g.drawText(name.isEmpty() ? TRANS("Preset sem nome") : name, r.reduced(36.0f, 0.0f), juce::Justification::centred, true);
  // Seta do menu à direita.
  juce::Path arrow;
  const float x = r.getRight() - 20.0f, y = r.getCentreY();
  arrow.addTriangle(x - 4.0f, y - 2.0f, x + 4.0f, y - 2.0f, x, y + 3.0f);
  g.setColour(colour::dim);
  g.fillPath(arrow);
}

void PresetBar::mouseUp(const juce::MouseEvent& e)
{
  if (nameArea().contains(e.getPosition()))
    showMenu();
}
} // namespace nexus::ui
