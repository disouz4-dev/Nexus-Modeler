#include "PackMapper.h"

#include <map>

#include "../tone3000/Tone3000.h"
#include "Language.h"
#include "Theme.h"
#include "nexus/Pack.h"

namespace nexus::ui
{
namespace
{
juce::String tr(const char* text)
{
  return TRANS(juce::String::fromUTF8(text));
}

// Posições do botão oferecidas: 0 a 10 de meio em meio.
constexpr int kSteps = 21;
double stepValue(int i)
{
  return i * 0.5;
}

juce::String positionKey(double p)
{
  const bool whole = std::abs(p - std::round(p)) < 1e-6;
  return whole ? juce::String(juce::roundToInt(p)) : juce::String(p, 1);
}

juce::Array<juce::File> namFiles(const juce::File& folder)
{
  auto files = folder.findChildFiles(juce::File::findFiles, false, "*.nam");
  files.sort();
  return files;
}

// A pasta com mais capturas dentro de "root" (o .zip costuma ter uma pasta por dentro).
juce::File folderWithCaptures(const juce::File& root)
{
  juce::File best;
  int most = 0;
  auto consider = [&](const juce::File& f) {
    const int n = namFiles(f).size();
    if (n > most)
    {
      most = n;
      best = f;
    }
  };
  consider(root);
  for (const auto& sub : root.findChildFiles(juce::File::findDirectories, true))
    if (!sub.getFileName().startsWith("__MACOSX"))
      consider(sub);
  return best;
}

// Nome livre dentro da pasta do equipamento ("Pacote", "Pacote 2"...).
juce::File freshFolder(const juce::File& parent, const juce::String& name)
{
  auto base = juce::File::createLegalFileName(name).trim();
  if (base.isEmpty())
    base = "Pacote";
  auto target = parent.getChildFile(base);
  for (int i = 2; target.exists(); ++i)
    target = parent.getChildFile(base + " " + juce::String(i));
  return target;
}
} // namespace

// ------------------------------------------------------------------ packs

namespace packs
{
std::vector<MappedCapture> analyze(const juce::File& folder, bool& needsMapping)
{
  std::vector<MappedCapture> out;
  needsMapping = false;
  const auto files = namFiles(folder);
  for (const auto& f : files)
    out.push_back({f, -1.0});
  if (out.empty())
    return out;

  const auto manifest = juce::JSON::parse(folder.getChildFile("pack.json"));
  if (manifest.isObject() && static_cast<bool>(manifest["mapear"]))
    needsMapping = true;
  if (auto* map = manifest["capturas"].getDynamicObject(); map != nullptr && map->getProperties().size() > 0)
  {
    // O pack.json manda: só as capturas listadas entram.
    for (const auto& p : map->getProperties())
      for (auto& c : out)
        if (c.file.getFileName() == p.value.toString())
          c.position = p.name.toString().getDoubleValue();
    return out;
  }
  // Pelo nome. Precisa de mapeamento se algum nome não tiver número ou se
  // duas capturas caírem na mesma posição.
  std::map<long, int> seen;
  for (auto& c : out)
  {
    double p = 0.0;
    if (nexus::positionFromFileName(c.file.getFileNameWithoutExtension().toStdString(), p))
    {
      c.position = p;
      if (++seen[std::lround(p * 100.0)] > 1)
        needsMapping = true;
    }
    else
      needsMapping = true;
  }
  return out;
}

juce::File importInto(const juce::Array<juce::File>& chosen, const juce::String& gearName, juce::String& error)
{
  const auto root = t3k::Connection::packsFolder(gearName);
  if (!root.createDirectory())
  {
    error = tr("Não foi possível criar a pasta %1").replace("%1", root.getFullPathName());
    return {};
  }
  if (chosen.isEmpty())
    return {};

  const auto first = chosen.getFirst();
  // Um .zip: descompacta dentro da pasta do equipamento.
  if (first.hasFileExtension("zip"))
  {
    juce::ZipFile zip(first);
    if (zip.getNumEntries() == 0)
    {
      error = tr("O arquivo .zip está vazio ou estragado.");
      return {};
    }
    const auto target = freshFolder(root, first.getFileNameWithoutExtension());
    if (zip.uncompressTo(target, true).failed())
    {
      target.deleteRecursively();
      error = tr("Não foi possível descompactar %1").replace("%1", first.getFileName());
      return {};
    }
    auto found = folderWithCaptures(target);
    // O .zip trazia uma pasta só ("Meu Amp/..."): sobe o conteúdo, sem pasta repetida.
    if (found.getParentDirectory() == target && target.getNumberOfChildFiles(juce::File::findFilesAndDirectories) == 1)
    {
      const auto temp = root.getNonexistentChildFile(".nexus_tmp", "");
      if (found.moveFileTo(temp) && target.deleteRecursively() && temp.moveFileTo(target))
        found = target;
    }
    if (!found.isDirectory())
    {
      target.deleteRecursively();
      error = tr("O .zip não tem capturas .nam.");
    }
    return found;
  }
  // Uma pasta: se já está na pasta do equipamento, usa como está; senão copia.
  if (first.isDirectory())
  {
    const auto source = folderWithCaptures(first);
    if (!source.isDirectory())
    {
      error = tr("A pasta não tem capturas .nam.");
      return {};
    }
    if (source.isAChildOf(t3k::Connection::packsFolder()))
      return source;
    const auto target = freshFolder(root, source.getFileName());
    if (!source.copyDirectoryTo(target))
    {
      error = tr("Não foi possível copiar a pasta para %1").replace("%1", root.getFullPathName());
      return {};
    }
    return target;
  }
  // Arquivos .nam soltos: vão juntos para uma pasta nova, com o nome da pasta de origem.
  const auto target = freshFolder(root, first.getParentDirectory().getFileName());
  target.createDirectory();
  int copied = 0;
  for (const auto& f : chosen)
    if (f.hasFileExtension("nam") && f.copyFileTo(target.getChildFile(f.getFileName())))
      ++copied;
  if (copied == 0)
  {
    target.deleteRecursively();
    error = tr("Escolha capturas .nam, uma pasta ou um .zip.");
    return {};
  }
  // pack.json da origem (se houver) vem junto.
  first.getSiblingFile("pack.json").copyFileTo(target.getChildFile("pack.json"));
  return target;
}

bool saveMapping(const juce::File& folder, const std::vector<MappedCapture>& captures, const juce::String& knobName)
{
  auto manifest = juce::JSON::parse(folder.getChildFile("pack.json"));
  auto* object = manifest.getDynamicObject();
  if (object == nullptr)
  {
    object = new juce::DynamicObject();
    manifest = juce::var(object);
    object->setProperty("nome", folder.getFileName());
  }
  auto* map = new juce::DynamicObject();
  for (const auto& c : captures)
    if (c.position >= 0.0)
      map->setProperty(positionKey(c.position), c.file.getFileName());
  if (map->getProperties().isEmpty())
    return false;
  object->setProperty("capturas", juce::var(map));
  object->removeProperty("mapear");
  if (!object->hasProperty("botao"))
    object->setProperty("botao", knobName);
  return folder.getChildFile("pack.json").replaceWithText(juce::JSON::toString(manifest));
}
} // namespace packs

// ------------------------------------------------------------------ Row

// Uma linha: nome da captura e a posição escolhida.
class PackMapper::Row : public juce::Component
{
public:
  Row(PackMapper& owner, size_t index) : mOwner(owner), mIndex(index)
  {
    addAndMakeVisible(mChoice);
    mChoice.addItem(tr("Não usar"), 1);
    for (int i = 0; i < kSteps; ++i)
      mChoice.addItem(positionKey(stepValue(i)).replaceCharacter('.', ','), i + 2);
    mChoice.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff131417));
    mChoice.setColour(juce::ComboBox::outlineColourId, colour::boxBorder);
    mChoice.setColour(juce::ComboBox::textColourId, colour::text);
    mChoice.setColour(juce::ComboBox::arrowColourId, colour::dim);
    mChoice.onChange = [this] {
      const int id = mChoice.getSelectedId();
      mOwner.mCaptures[mIndex].position = id <= 1 ? -1.0 : stepValue(id - 2);
      mOwner.validate();
    };
    refresh();
  }

  void refresh()
  {
    const double p = mOwner.mCaptures[mIndex].position;
    mChoice.setSelectedId(p < 0.0 ? 1 : juce::jlimit(0, kSteps - 1, juce::roundToInt(p * 2.0)) + 2, juce::dontSendNotification);
    repaint();
  }

  void setDuplicate(bool d)
  {
    if (d != mDuplicate)
    {
      mDuplicate = d;
      repaint();
    }
  }

  void paint(juce::Graphics& g) override
  {
    auto r = getLocalBounds().toFloat().reduced(0.0f, 3.0f);
    g.setColour(juce::Colour(0xff131417));
    g.fillRoundedRectangle(r, 8.0f);
    if (mDuplicate)
    {
      g.setColour(colour::danger.withAlpha(0.7f));
      g.drawRoundedRectangle(r.reduced(0.5f), 8.0f, 1.0f);
    }
    g.setColour(mOwner.mCaptures[mIndex].position < 0.0 ? colour::dim : colour::text);
    g.setFont(Fonts::get().ui(13.5f));
    g.drawText(mOwner.mCaptures[mIndex].file.getFileNameWithoutExtension(), r.reduced(16.0f, 0.0f).withTrimmedRight(150.0f),
               juce::Justification::centredLeft, true);
  }

  void resized() override { mChoice.setBounds(getLocalBounds().removeFromRight(140).reduced(12, 9)); }

private:
  PackMapper& mOwner;
  size_t mIndex;
  juce::ComboBox mChoice;
  bool mDuplicate = false;
};

// ------------------------------------------------------------------ PackMapper

PackMapper::PackMapper()
{
  mViewport.setViewedComponent(&mRows, false);
  mViewport.setScrollBarsShown(true, false);
  addAndMakeVisible(mViewport);
  for (auto* b : {&mSpread, &mSave, &mCancel})
    addAndMakeVisible(b);
  mSpread.onClick = [this] { spread(); };
  mCancel.onClick = [this] {
    if (onClose)
      onClose();
  };
  mSave.onClick = [this] {
    if (packs::saveMapping(mFolder, mCaptures, mKnobName) && onSaved)
      onSaved(mFolder);
  };
}

PackMapper::~PackMapper() = default;

void PackMapper::open(const juce::File& folder, const juce::String& gearName, const juce::String& knobName)
{
  mFolder = folder;
  mGearName = gearName;
  mKnobName = knobName;
  bool unused = false;
  mCaptures = packs::analyze(folder, unused);
  mRowViews.clear();
  for (size_t i = 0; i < mCaptures.size(); ++i)
  {
    mRowViews.push_back(std::make_unique<Row>(*this, i));
    mRows.addAndMakeVisible(*mRowViews.back());
  }
  retranslate();
  resized();
  validate();
}

void PackMapper::retranslate()
{
  mSpread.setButtonText(tr("Distribuir de 0 a 10"));
  mSave.setButtonText(tr("Salvar e carregar"));
  mCancel.setButtonText(tr("Cancelar"));
  repaint();
}

// Posições em ordem pelo nome, de 0 a 10 (para pacotes sem número no nome).
void PackMapper::spread()
{
  const int n = static_cast<int>(mCaptures.size());
  for (int i = 0; i < n; ++i)
  {
    const double p = n == 1 ? 5.0 : 10.0 * i / (n - 1);
    mCaptures[static_cast<size_t>(i)].position = std::round(p * 2.0) / 2.0;
  }
  for (auto& r : mRowViews)
    r->refresh();
  validate();
}

void PackMapper::validate()
{
  std::map<long, int> count;
  int used = 0;
  for (const auto& c : mCaptures)
    if (c.position >= 0.0)
    {
      ++count[std::lround(c.position * 100.0)];
      ++used;
    }
  bool duplicate = false;
  for (size_t i = 0; i < mCaptures.size(); ++i)
  {
    const auto p = mCaptures[i].position;
    const bool d = p >= 0.0 && count[std::lround(p * 100.0)] > 1;
    duplicate = duplicate || d;
    mRowViews[i]->setDuplicate(d);
  }
  mProblem = used == 0     ? tr("Escolha a posição de pelo menos uma captura.")
             : duplicate   ? tr("Duas capturas estão na mesma posição.")
                           : juce::String();
  mSave.setEnabled(mProblem.isEmpty());
  repaint();
}

juce::Rectangle<int> PackMapper::card() const
{
  return getLocalBounds().withSizeKeepingCentre(std::min(720, getWidth() - 40), std::min(600, getHeight() - 40));
}

void PackMapper::resized()
{
  auto r = card().reduced(32, 0);
  r.removeFromTop(120);
  auto bottom = r.removeFromBottom(76);
  r.removeFromBottom(30);
  mViewport.setBounds(r);
  const int rowH = 52;
  mRows.setSize(r.getWidth() - mViewport.getScrollBarThickness() - 4, rowH * static_cast<int>(mRowViews.size()));
  for (size_t i = 0; i < mRowViews.size(); ++i)
    mRowViews[i]->setBounds(0, rowH * static_cast<int>(i), mRows.getWidth(), rowH);
  auto buttons = bottom.withTrimmedTop(20).withTrimmedBottom(20);
  mSave.setBounds(buttons.removeFromRight(180));
  buttons.removeFromRight(10);
  mCancel.setBounds(buttons.removeFromRight(110));
  mSpread.setBounds(buttons.removeFromLeft(200));
}

void PackMapper::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colours::black.withAlpha(0.6f));
  const auto c = card().toFloat();
  g.setColour(juce::Colour(0xff17181b));
  g.fillRoundedRectangle(c, 12.0f);
  g.setColour(colour::boxBorder);
  g.drawRoundedRectangle(c.reduced(0.5f), 12.0f, 1.0f);

  auto r = c.reduced(32.0f, 0.0f);
  auto head = r.removeFromTop(120.0f);
  g.setColour(colour::text);
  g.setFont(Fonts::get().display(20.0f).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.14f : 0.0f));
  g.drawText(upper(tr("Mapear as capturas")), head.withTrimmedTop(14.0f).removeFromTop(30.0f), juce::Justification::centredLeft);
  g.setColour(colour::accent);
  g.fillRect(head.getX(), head.getY() + 50.0f, 28.0f, 2.0f);
  g.setColour(colour::dim);
  g.setFont(Fonts::get().ui(13.0f));
  g.drawFittedText(tr("O Nexus não descobriu sozinho a posição do botão %1 de cada captura de \"%2\". Escolha a posição em que cada uma foi gravada; as que ficarem em \"Não usar\" não entram.")
                     .replace("%1", mKnobName)
                     .replace("%2", mFolder.getFileName()),
                   head.withTrimmedTop(62.0f).toNearestInt(), juce::Justification::topLeft, 3);

  auto status = juce::Rectangle<float>(r.getX(), static_cast<float>(mViewport.getBottom()) + 6.0f, r.getWidth(), 22.0f);
  g.setColour(mProblem.isNotEmpty() ? colour::danger : colour::faint);
  g.setFont(Fonts::get().ui(12.0f));
  g.drawFittedText(mProblem.isNotEmpty() ? mProblem : tr("O mapeamento fica gravado no pack.json da pasta do pacote."),
                   status.toNearestInt(), juce::Justification::centredLeft, 1);
}

void PackMapper::mouseUp(const juce::MouseEvent& e)
{
  if (!card().contains(e.getPosition()) && onClose)
    onClose();
}
} // namespace nexus::ui
