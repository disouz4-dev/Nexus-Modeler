#include "StudioView.h"

#include "Theme.h"

namespace nexus::ui
{
namespace
{
using nexus::studio::kStems;

juce::String tr(const char* text)
{
  return TRANS(juce::String::fromUTF8(text));
}

// Cor de cada faixa (na ordem do modelo: bateria, baixo, resto, voz, guitarra, piano).
juce::Colour stemColour(int stem)
{
  static const juce::Colour c[] = {juce::Colour(0xfff0a04b), juce::Colour(0xffa98bff), juce::Colour(0xff8e9aa8),
                                   juce::Colour(0xffff7aa8), colour::accent, juce::Colour(0xff6fd39a)};
  return c[juce::jlimit(0, kStems - 1, stem)];
}

constexpr float kSidebarWidth = 300.0f;
constexpr float kPad = 18.0f;
} // namespace

StudioView::StudioView(nexus::studio::Player& player, nexus::studio::Separation& separation)
: mPlayer(player)
, mSeparation(separation)
{
  setWantsKeyboardFocus(true);
  addAndMakeVisible(mTabs);
  mTabs.setFontSize(11.0f);
  mTabs.onSelect = [this](int i) {
    mTab = i;
    mOpenPlaylist = -1;
    mScroll = 0;
    mTabs.setSelected(i);
    resized();
    repaint();
  };
  mLibrary.rescan();
  // Volta para a música que estava aberta (a tela pode ter sido fechada).
  if (mPlayer.isOpen() && mLibrary.find(mPlayer.songId()) == nullptr)
    mPlayer.close();
  retranslate();
  startTimerHz(30);
}

StudioView::~StudioView() = default;

void StudioView::retranslate()
{
  mTabs.setItems({tr("Músicas"), tr("Playlists")});
  repaint();
}

const nexus::studio::Song* StudioView::current() const
{
  return mPlayer.isOpen() ? mLibrary.find(mPlayer.songId()) : nullptr;
}

juce::StringArray StudioView::rowIds() const
{
  juce::StringArray ids;
  if (mTab == 0)
    for (const auto& s : mLibrary.songs())
      ids.add(s.id());
  else if (mOpenPlaylist < 0)
    for (size_t i = 0; i < mLibrary.playlists().size(); ++i)
      ids.add(juce::String(static_cast<int>(i)));
  else if (mOpenPlaylist < static_cast<int>(mLibrary.playlists().size()))
    ids = mLibrary.playlists()[static_cast<size_t>(mOpenPlaylist)].songs;
  return ids;
}

juce::String StudioView::formatTime(double seconds) const
{
  const int s = juce::jmax(0, static_cast<int>(seconds));
  return juce::String(s / 60) + ":" + juce::String(s % 60).paddedLeft('0', 2);
}

juce::String StudioView::stageText() const
{
  using Stage = nexus::studio::Separation::Stage;
  const int pc = juce::roundToInt(mSeparation.progress() * 100.0f);
  switch (mSeparation.stage())
  {
    case Stage::downloading:
      return tr("Baixando o separador (55 MB, só na primeira vez)... %1%").replace("%1", juce::String(pc));
    case Stage::reading:
      return tr("Lendo a música... %1%").replace("%1", juce::String(pc));
    case Stage::separating:
    {
      auto text = tr("Separando as faixas... %1%").replace("%1", juce::String(pc));
      const double left = mSeparation.remainingSeconds();
      if (left > 0.0)
        text << "  ·  " << (left < 90.0 ? tr("falta menos de 2 min")
                                        : tr("faltam ~%1 min").replace("%1", juce::String(juce::roundToInt(left / 60.0))));
      return text;
    }
    case Stage::saving:
      return tr("Gravando as faixas...");
    case Stage::idle:
    case Stage::done:
    case Stage::failed:
      break;
  }
  return {};
}

// ------------------------------------------------------------------ layout

void StudioView::resized()
{
  auto r = getLocalBounds().toFloat();
  mSidebar = r.removeFromLeft(kSidebarWidth);
  mMain = r.reduced(28.0f, 20.0f);

  auto s = mSidebar.reduced(kPad, kPad);
  mTabsArea = s.removeFromTop(34.0f);
  mTabs.setBounds(mTabsArea.toNearestInt());
  s.removeFromTop(12.0f);
  auto action = s.removeFromTop(34.0f);
  mImportArea = mNewPlaylistArea = mBackArea = mPlayPlaylistArea = {};
  if (mTab == 0)
    mImportArea = action;
  else if (mOpenPlaylist < 0)
    mNewPlaylistArea = action;
  else
  {
    mBackArea = action.removeFromLeft(action.getWidth() * 0.42f).reduced(0.0f, 0.0f);
    action.removeFromLeft(8.0f);
    mPlayPlaylistArea = action;
  }
  s.removeFromTop(10.0f);
  mJobArea = mCancelArea = {};
  if (mSeparation.busy())
  {
    mJobArea = s.removeFromBottom(92.0f);
    mCancelArea = mJobArea.reduced(12.0f).removeFromBottom(26.0f).removeFromRight(96.0f);
    s.removeFromBottom(10.0f);
  }
  mList = s;

  // Centro.
  auto m = mMain;
  mEmptyButton = m.withSizeKeepingCentre(220.0f, 40.0f).translated(0.0f, 46.0f);
  mHeader = m.removeFromTop(44.0f);
  {
    auto h = mHeader;
    mLoopArea = h.removeFromRight(150.0f).withSizeKeepingCentre(150.0f, 34.0f);
    h.removeFromRight(10.0f);
    mPlayArea = h.removeFromRight(44.0f).withSizeKeepingCentre(44.0f, 44.0f);
    h.removeFromRight(6.0f);
    mRewindArea = h.removeFromRight(36.0f).withSizeKeepingCentre(36.0f, 36.0f);
    h.removeFromRight(16.0f);
    mTimeArea = h.removeFromRight(110.0f);
  }
  m.removeFromTop(12.0f);
  mWaveArea = m.removeFromTop(64.0f);
  m.removeFromTop(14.0f);
  mBottom = m.removeFromBottom(40.0f);
  m.removeFromBottom(12.0f);
  mStemsArea = m;
  const float rowH = std::min(52.0f, mStemsArea.getHeight() / kStems);
  auto rows = mStemsArea;
  for (int i = 0; i < kStems; ++i)
    mStemRows[static_cast<size_t>(i)] = rows.removeFromTop(rowH);
  {
    auto b = mBottom;
    mSpeedArea = b.removeFromLeft(std::min(330.0f, b.getWidth() * 0.4f));
    b.removeFromLeft(36.0f);
    auto pitch = b.removeFromLeft(230.0f);
    mPitchArea = pitch;
    pitch.removeFromLeft(110.0f);
    mPitchDownArea = pitch.removeFromLeft(32.0f).withSizeKeepingCentre(32.0f, 32.0f);
    mPitchUpArea = pitch.removeFromRight(32.0f).withSizeKeepingCentre(32.0f, 32.0f);
    b.removeFromLeft(36.0f);
    mVolumeArea = b.removeFromLeft(std::min(300.0f, b.getWidth()));
  }
}

// ------------------------------------------------------------------ desenho

void StudioView::paintPill(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, bool on, bool hover,
                           juce::Colour onColour) const
{
  r = r.reduced(0.5f);
  g.setColour(on ? onColour.withAlpha(0.16f) : juce::Colour(hover ? 0xff1d1f24 : 0xff15161a));
  g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
  g.setColour(on ? onColour : (hover ? colour::dim : colour::boxBorder));
  g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
  drawCaps(g, text, r, 10.5f, juce::Justification::centred, on ? onColour : colour::text);
}

void StudioView::paintSlider(juce::Graphics& g, juce::Rectangle<float> r, float v, bool hover, float level) const
{
  const auto track = r.withSizeKeepingCentre(r.getWidth(), 4.0f);
  g.setColour(juce::Colour(0xff26272b));
  g.fillRoundedRectangle(track, 2.0f);
  if (level >= 0.0f)
  {
    // Medidor por baixo: o som da faixa.
    g.setColour(colour::accent.withAlpha(0.55f));
    g.fillRoundedRectangle(track.withWidth(track.getWidth() * juce::jlimit(0.0f, 1.0f, level)), 2.0f);
  }
  const float x = r.getX() + r.getWidth() * juce::jlimit(0.0f, 1.0f, v);
  g.setColour(hover ? colour::selected : colour::text);
  g.fillRoundedRectangle(x - 3.0f, r.getCentreY() - 9.0f, 6.0f, 18.0f, 3.0f);
}

void StudioView::paint(juce::Graphics& g)
{
  g.fillAll(colour::background);
  paintSidebar(g);
  if (const auto* song = current())
    paintSong(g, *song);
  else
    paintEmpty(g);
}

void StudioView::paintSidebar(juce::Graphics& g)
{
  g.setColour(colour::sidebar);
  g.fillRect(mSidebar);
  g.setColour(colour::line);
  g.drawVerticalLine(juce::roundToInt(mSidebar.getRight()) - 1, mSidebar.getY(), mSidebar.getBottom());

  if (!mImportArea.isEmpty())
    paintPill(g, mImportArea, "+  " + tr("Importar música"), false, mHover.kind == Hit::importButton, colour::accent);
  if (!mNewPlaylistArea.isEmpty())
    paintPill(g, mNewPlaylistArea, "+  " + tr("Nova playlist"), false, mHover.kind == Hit::newPlaylist, colour::accent);
  if (!mBackArea.isEmpty())
  {
    paintPill(g, mBackArea, juce::String::fromUTF8("‹  ") + tr("Playlists"), false, mHover.kind == Hit::playlistBack, colour::accent);
    paintPill(g, mPlayPlaylistArea, tr("Tocar a playlist"), mPlayingPlaylist == mOpenPlaylist && mPlayer.playing(),
              mHover.kind == Hit::playPlaylist, colour::accent);
  }

  // Lista.
  const auto ids = rowIds();
  g.saveState();
  g.reduceClipRegion(mList.toNearestInt());
  const auto openId = mPlayer.songId();
  for (int i = mScroll; i < ids.size(); ++i)
  {
    auto row = juce::Rectangle<float>(mList.getX(), mList.getY() + (i - mScroll) * kRowHeight, mList.getWidth(), kRowHeight - 4.0f);
    if (row.getY() > mList.getBottom())
      break;
    const bool hover = mHover.kind == Hit::listRow && mHover.index == i;
    juce::String title, detail;
    bool active = false;
    if (mTab == 1 && mOpenPlaylist < 0)
    {
      const auto& p = mLibrary.playlists()[static_cast<size_t>(ids[i].getIntValue())];
      title = p.name;
      detail = p.songs.size() == 1 ? tr("1 música") : tr("%1 músicas").replace("%1", juce::String(p.songs.size()));
      active = mPlayingPlaylist == ids[i].getIntValue();
    }
    else if (const auto* s = mLibrary.find(ids[i]))
    {
      title = s->title;
      detail = formatTime(s->seconds);
      active = s->id() == openId;
    }
    if (active || hover)
    {
      g.setColour(active ? juce::Colour(0xff17222c) : colour::boxHover);
      g.fillRoundedRectangle(row, 8.0f);
    }
    if (active)
    {
      g.setColour(colour::accent);
      g.fillRoundedRectangle(row.getX(), row.getY() + 8.0f, 3.0f, row.getHeight() - 16.0f, 1.5f);
    }
    auto t = row.reduced(14.0f, 0.0f);
    g.setFont(Fonts::get().ui(12.0f));
    g.setColour(colour::dim);
    const auto d = t.removeFromRight(70.0f);
    g.drawText(detail, d, juce::Justification::centredRight);
    g.setFont(Fonts::get().ui(13.5f));
    g.setColour(active ? colour::selected : colour::text);
    g.drawText(title, t, juce::Justification::centredLeft, true);
  }
  if (ids.isEmpty())
  {
    g.setFont(Fonts::get().ui(12.5f));
    g.setColour(colour::faint);
    const auto text = mTab == 0 ? tr("Nenhuma música ainda.")
                                : (mOpenPlaylist < 0 ? tr("Nenhuma playlist ainda.")
                                                     : tr("Playlist vazia. Na aba Músicas, clique com o botão direito numa música para pôr aqui."));
    g.drawFittedText(text, mList.removeFromTop(60.0f).reduced(4.0f, 0.0f).toNearestInt(), juce::Justification::topLeft, 3);
  }
  g.restoreState();

  // Andamento da separação.
  if (!mJobArea.isEmpty())
  {
    auto r = mJobArea;
    g.setColour(juce::Colour(0xff15181d));
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(colour::boxBorder);
    g.drawRoundedRectangle(r.reduced(0.5f), 10.0f, 1.0f);
    auto in = r.reduced(12.0f);
    g.setFont(Fonts::get().ui(12.5f));
    g.setColour(colour::text);
    g.drawText(mSeparation.title(), in.removeFromTop(18.0f), juce::Justification::centredLeft, true);
    g.setFont(Fonts::get().ui(11.5f));
    g.setColour(colour::dim);
    g.drawText(stageText(), in.removeFromTop(18.0f), juce::Justification::centredLeft, true);
    in.removeFromTop(4.0f);
    auto bar = in.removeFromTop(4.0f);
    g.setColour(juce::Colour(0xff26272b));
    g.fillRoundedRectangle(bar, 2.0f);
    g.setColour(colour::accent);
    const bool model = mSeparation.stage() == nexus::studio::Separation::Stage::downloading;
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * juce::jlimit(0.0f, 1.0f, model ? mSeparation.progress() : mSeparation.overall())), 2.0f);
    paintPill(g, mCancelArea, tr("Cancelar"), false, mHover.kind == Hit::cancelButton, colour::danger);
  }
}

void StudioView::paintEmpty(juce::Graphics& g)
{
  auto r = mMain;
  const auto centre = r.getCentre();
  g.setFont(Fonts::get().display(30.0f).withExtraKerningFactor(0.08f));
  g.setColour(colour::text);
  g.drawText(upper(tr("Estúdio")), juce::Rectangle<float>(r.getX(), centre.y - 96.0f, r.getWidth(), 40.0f), juce::Justification::centred);
  g.setFont(Fonts::get().ui(14.0f));
  g.setColour(colour::dim);
  g.drawFittedText(tr("Importe uma música e o Nexus separa voz, guitarra, baixo, bateria, piano e o resto. "
                      "Tire a guitarra para tocar no lugar, desacelere um trecho difícil e monte playlists para ensaiar."),
                   juce::Rectangle<float>(r.getX() + r.getWidth() * 0.18f, centre.y - 46.0f, r.getWidth() * 0.64f, 60.0f).toNearestInt(),
                   juce::Justification::centredTop, 3);
  if (mSeparation.busy())
  {
    // Separando: o andamento grande no meio da tela.
    auto card = juce::Rectangle<float>(r.getX() + r.getWidth() * 0.2f, mEmptyButton.getY() - 6.0f, r.getWidth() * 0.6f, 96.0f);
    g.setColour(juce::Colour(0xff15181d));
    g.fillRoundedRectangle(card, 12.0f);
    g.setColour(colour::boxBorder);
    g.drawRoundedRectangle(card.reduced(0.5f), 12.0f, 1.0f);
    auto in = card.reduced(20.0f, 16.0f);
    g.setFont(Fonts::get().ui(14.5f));
    g.setColour(colour::text);
    g.drawText(mSeparation.title(), in.removeFromTop(22.0f), juce::Justification::centredLeft, true);
    g.setFont(Fonts::get().ui(12.5f));
    g.setColour(colour::dim);
    g.drawText(stageText(), in.removeFromTop(22.0f), juce::Justification::centredLeft, true);
    in.removeFromTop(10.0f);
    auto bar = in.removeFromTop(6.0f);
    g.setColour(juce::Colour(0xff26272b));
    g.fillRoundedRectangle(bar, 3.0f);
    g.setColour(colour::accent);
    const bool model = mSeparation.stage() == nexus::studio::Separation::Stage::downloading;
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * juce::jlimit(0.0f, 1.0f, model ? mSeparation.progress() : mSeparation.overall())), 3.0f);
    return;
  }
  paintPill(g, mEmptyButton, "+  " + tr("Importar música"), true, mHover.kind == Hit::emptyImport, colour::accent);
  g.setFont(Fonts::get().ui(11.5f));
  g.setColour(colour::faint);
  g.drawText(tr("ou arraste o arquivo para cá (MP3, WAV, FLAC, M4A)"),
             mEmptyButton.translated(0.0f, 40.0f).withSizeKeepingCentre(500.0f, 20.0f), juce::Justification::centred);
}

void StudioView::paintSong(juce::Graphics& g, const nexus::studio::Song& song)
{
  const auto& mix = song.mix;
  // Cabeçalho.
  {
    auto h = mHeader;
    g.setFont(Fonts::get().display(24.0f).withExtraKerningFactor(0.04f));
    g.setColour(colour::text);
    g.drawText(upper(song.title), h.withRight(mTimeArea.getX() - 12.0f), juce::Justification::centredLeft, true);
    g.setFont(Fonts::get().ui(13.0f));
    g.setColour(colour::dim);
    g.drawText(formatTime(mPlayer.position()) + " / " + formatTime(mPlayer.length()), mTimeArea, juce::Justification::centredRight);

    // Voltar ao começo (ou ao A do trecho).
    {
      const auto r = mRewindArea;
      const bool hover = mHover.kind == Hit::rewind;
      g.setColour(hover ? colour::text : colour::dim);
      const float s = 10.0f;
      juce::Path p;
      p.addRectangle(r.getCentreX() - s * 0.75f, r.getCentreY() - s * 0.6f, 2.0f, s * 1.2f);
      p.addTriangle(r.getCentreX() + s * 0.6f, r.getCentreY() - s * 0.6f, r.getCentreX() + s * 0.6f, r.getCentreY() + s * 0.6f,
                    r.getCentreX() - s * 0.45f, r.getCentreY());
      g.fillPath(p);
    }
    // Tocar / pausar.
    {
      const auto r = mPlayArea.reduced(1.0f);
      const bool hover = mHover.kind == Hit::play;
      g.setColour(hover ? colour::accent.brighter(0.2f) : colour::accent);
      g.fillEllipse(r);
      g.setColour(juce::Colour(0xff0b1620));
      const float s = 9.0f;
      if (mPlayer.playing())
      {
        g.fillRoundedRectangle(r.getCentreX() - s * 0.7f, r.getCentreY() - s, s * 0.5f, s * 2.0f, 1.5f);
        g.fillRoundedRectangle(r.getCentreX() + s * 0.2f, r.getCentreY() - s, s * 0.5f, s * 2.0f, 1.5f);
      }
      else
      {
        juce::Path p;
        p.addTriangle(r.getCentreX() - s * 0.55f, r.getCentreY() - s, r.getCentreX() - s * 0.55f, r.getCentreY() + s,
                      r.getCentreX() + s * 0.95f, r.getCentreY());
        g.fillPath(p);
      }
    }
    paintPill(g, mLoopArea, tr("Repetir A–B"), mix.loop, mHover.kind == Hit::loopButton, colour::accent);
  }

  // Forma da música, trecho A–B e o ponto que toca.
  {
    auto w = mWaveArea;
    g.setColour(juce::Colour(0xff111215));
    g.fillRoundedRectangle(w, 8.0f);
    const double len = std::max(0.001, mPlayer.length());
    const auto xOf = [&](double sec) { return w.getX() + static_cast<float>(sec / len) * w.getWidth(); };
    if (mix.loopEnd > mix.loopStart + 0.25)
    {
      const float a = xOf(mix.loopStart), b = xOf(mix.loopEnd);
      g.setColour(colour::accent.withAlpha(mix.loop ? 0.16f : 0.07f));
      g.fillRect(a, w.getY(), b - a, w.getHeight());
      g.setColour(colour::accent.withAlpha(mix.loop ? 0.9f : 0.4f));
      g.fillRect(a, w.getY(), 1.5f, w.getHeight());
      g.fillRect(b - 1.5f, w.getY(), 1.5f, w.getHeight());
      g.setFont(Fonts::get().ui(10.0f));
      g.drawText("A", juce::Rectangle<float>(a + 4.0f, w.getY() + 2.0f, 12.0f, 12.0f), juce::Justification::centredLeft);
      g.drawText("B", juce::Rectangle<float>(b - 14.0f, w.getY() + 2.0f, 12.0f, 12.0f), juce::Justification::centredRight);
    }
    const float play = xOf(mPlayer.position());
    const auto& shape = song.shape;
    if (!shape.empty())
    {
      const int bars = juce::jmax(1, static_cast<int>(w.getWidth() / 3.0f));
      for (int i = 0; i < bars; ++i)
      {
        const float v = shape[static_cast<size_t>(i) * shape.size() / static_cast<size_t>(bars)];
        const float x = w.getX() + (i + 0.5f) * w.getWidth() / bars;
        const float hh = std::max(1.5f, v * (w.getHeight() * 0.5f - 6.0f));
        g.setColour(x < play ? colour::accent : juce::Colour(0xff4a4d55));
        g.fillRoundedRectangle(x - 1.0f, w.getCentreY() - hh, 2.0f, hh * 2.0f, 1.0f);
      }
    }
    g.setColour(colour::selected);
    g.fillRect(play - 1.0f, w.getY(), 2.0f, w.getHeight());
  }

  // Faixas.
  for (int row = 0; row < kStems; ++row)
  {
    const int stem = nexus::studio::displayOrder()[static_cast<size_t>(row)];
    auto r = mStemRows[static_cast<size_t>(row)].reduced(0.0f, 3.0f);
    const auto c = stemColour(stem);
    const bool muted = mix.mute[static_cast<size_t>(stem)];
    bool anySolo = false;
    for (auto s : mix.solo)
      anySolo = anySolo || s;
    const bool audible = !muted && (!anySolo || mix.solo[static_cast<size_t>(stem)]);
    if (row % 2 == 0)
    {
      g.setColour(juce::Colour(0xff17181b));
      g.fillRoundedRectangle(r, 6.0f);
    }
    auto name = r.removeFromLeft(150.0f).reduced(14.0f, 0.0f);
    g.setColour(audible ? c : c.withAlpha(0.3f));
    g.fillEllipse(name.getX(), name.getCentreY() - 4.0f, 8.0f, 8.0f);
    name.removeFromLeft(18.0f);
    g.setFont(Fonts::get().ui(13.0f));
    g.setColour(audible ? colour::text : colour::faint);
    g.drawText(tr(nexus::studio::stemName(stem).toRawUTF8()), name, juce::Justification::centredLeft);

    auto solo = r.removeFromRight(40.0f).withSizeKeepingCentre(30.0f, 24.0f);
    r.removeFromRight(4.0f);
    auto mute = r.removeFromRight(40.0f).withSizeKeepingCentre(30.0f, 24.0f);
    r.removeFromRight(18.0f);
    paintPill(g, mute, "M", muted, mHover.kind == Hit::stemMute && mHover.index == stem, colour::danger);
    paintPill(g, solo, "S", mix.solo[static_cast<size_t>(stem)], mHover.kind == Hit::stemSolo && mHover.index == stem,
              juce::Colour(0xfff5c84b));
    const float vol01 = mix.volume[static_cast<size_t>(stem)] / 1.5f;
    paintSlider(g, r.reduced(4.0f, 0.0f), vol01, mHover.kind == Hit::stemVolume && mHover.index == stem,
                audible ? mLevels[static_cast<size_t>(stem)] : 0.0f);
  }

  // Velocidade, tom e volume.
  {
    auto label = [&](juce::Rectangle<float>& area, const juce::String& text, const juce::String& value) {
      auto top = area.removeFromLeft(110.0f);
      drawCaps(g, text, top.removeFromTop(top.getHeight() * 0.5f + 2.0f), 10.0f, juce::Justification::bottomLeft, colour::dim);
      g.setFont(Fonts::get().ui(13.0f));
      g.setColour(colour::text);
      g.drawText(value, top, juce::Justification::topLeft);
    };
    auto sp = mSpeedArea;
    label(sp, tr("Velocidade"), juce::String(juce::roundToInt(mix.speed * 100.0)) + "%");
    paintSlider(g, sp, static_cast<float>((mix.speed - 0.5) / 0.5), mHover.kind == Hit::speed);

    auto pt = mPitchArea;
    label(pt, tr("Tom"), mix.semitones == 0 ? tr("original") : tr("%1 semitons").replace("%1", (mix.semitones > 0 ? "+" : "") + juce::String(mix.semitones)));
    paintPill(g, mPitchDownArea, juce::String::fromUTF8("−"), false, mHover.kind == Hit::pitchDown, colour::accent);
    paintPill(g, mPitchUpArea, "+", false, mHover.kind == Hit::pitchUp, colour::accent);
    g.setFont(Fonts::get().ui(15.0f));
    g.setColour(mix.semitones == 0 ? colour::dim : colour::accent);
    g.drawText((mix.semitones > 0 ? "+" : "") + juce::String(mix.semitones),
               juce::Rectangle<float>(mPitchDownArea.getRight(), mPitchArea.getY(), mPitchUpArea.getX() - mPitchDownArea.getRight(), mPitchArea.getHeight()),
               juce::Justification::centred);

    auto vo = mVolumeArea;
    label(vo, tr("Volume da música"), juce::String(juce::roundToInt(mMusicVolume * 100.0f)) + "%");
    paintSlider(g, vo, mMusicVolume, mHover.kind == Hit::volume);
  }
}

// ------------------------------------------------------------------ mouse

StudioView::Hit StudioView::hitAt(juce::Point<float> p) const
{
  if (mImportArea.contains(p))
    return {Hit::importButton};
  if (mNewPlaylistArea.contains(p))
    return {Hit::newPlaylist};
  if (mBackArea.contains(p))
    return {Hit::playlistBack};
  if (mPlayPlaylistArea.contains(p))
    return {Hit::playPlaylist};
  if (mCancelArea.contains(p))
    return {Hit::cancelButton};
  if (mList.contains(p))
  {
    const int i = mScroll + static_cast<int>((p.y - mList.getY()) / kRowHeight);
    if (i >= 0 && i < rowIds().size())
      return {Hit::listRow, i};
    return {};
  }
  if (current() == nullptr)
    return mEmptyButton.contains(p) && !mSeparation.busy() ? Hit{Hit::emptyImport} : Hit{};
  if (mPlayArea.contains(p))
    return {Hit::play};
  if (mRewindArea.contains(p))
    return {Hit::rewind};
  if (mLoopArea.contains(p))
    return {Hit::loopButton};
  if (mWaveArea.contains(p))
    return {Hit::wave};
  for (int row = 0; row < kStems; ++row)
  {
    auto r = mStemRows[static_cast<size_t>(row)];
    if (!r.contains(p))
      continue;
    const int stem = nexus::studio::displayOrder()[static_cast<size_t>(row)];
    r.removeFromLeft(150.0f);
    if (r.removeFromRight(40.0f).contains(p))
      return {Hit::stemSolo, stem};
    r.removeFromRight(4.0f);
    if (r.removeFromRight(40.0f).contains(p))
      return {Hit::stemMute, stem};
    r.removeFromRight(18.0f);
    if (r.contains(p))
      return {Hit::stemVolume, stem};
    return {};
  }
  if (mPitchDownArea.contains(p))
    return {Hit::pitchDown};
  if (mPitchUpArea.contains(p))
    return {Hit::pitchUp};
  if (mSpeedArea.withTrimmedLeft(110.0f).contains(p))
    return {Hit::speed};
  if (mVolumeArea.withTrimmedLeft(110.0f).contains(p))
    return {Hit::volume};
  return {};
}

void StudioView::mouseMove(const juce::MouseEvent& e)
{
  const auto h = hitAt(e.position);
  if (!(h == mHover))
  {
    mHover = h;
    setMouseCursor(h.kind == Hit::none ? juce::MouseCursor::NormalCursor : juce::MouseCursor::PointingHandCursor);
    repaint();
  }
}

void StudioView::mouseExit(const juce::MouseEvent&)
{
  mHover = {};
  repaint();
}

void StudioView::changeMix(const std::function<void(nexus::studio::Mix&)>& change, bool save)
{
  const auto* song = current();
  if (song == nullptr)
    return;
  auto mix = song->mix;
  change(mix);
  mPlayer.setMix(mix);
  // A biblioteca guarda a mistura (no disco só ao soltar o mouse).
  if (save)
    mLibrary.saveMix(song->id(), mix);
  else
    const_cast<nexus::studio::Song*>(song)->mix = mix;
  repaint();
}

void StudioView::mouseDown(const juce::MouseEvent& e)
{
  grabKeyboardFocus();
  mPressed = hitAt(e.position);
  mDraggedWave = false;
  if (e.mods.isPopupMenu())
  {
    if (mPressed.kind == Hit::listRow)
    {
      const auto ids = rowIds();
      if (mTab == 1 && mOpenPlaylist < 0)
        showPlaylistMenu(ids[mPressed.index].getIntValue());
      else
        showSongMenu(ids[mPressed.index]);
    }
    mPressed = {};
    return;
  }
  const auto* song = current();
  auto valueAt = [&](juce::Rectangle<float> r) { return juce::jlimit(0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth()); };
  switch (mPressed.kind)
  {
    case Hit::wave:
      mDragStart01 = valueAt(mWaveArea);
      break;
    case Hit::stemVolume:
    {
      auto r = mStemRows[0];
      r.removeFromLeft(150.0f);
      r.removeFromRight(102.0f);
      r = r.reduced(4.0f, 0.0f);
      const float v = valueAt(r) * 1.5f;
      const int stem = mPressed.index;
      changeMix([stem, v](auto& m) { m.volume[static_cast<size_t>(stem)] = v; }, false);
      break;
    }
    case Hit::speed:
    {
      const float v = valueAt(mSpeedArea.withTrimmedLeft(110.0f));
      changeMix([v](auto& m) { m.speed = 0.5 + std::round(v * 10.0) / 20.0; }, false);
      break;
    }
    case Hit::volume:
      mMusicVolume = valueAt(mVolumeArea.withTrimmedLeft(110.0f));
      mPlayer.setVolume(mMusicVolume * mMusicVolume * 1.25f);
      repaint();
      break;
    case Hit::none:
    case Hit::tab:
    case Hit::importButton:
    case Hit::cancelButton:
    case Hit::newPlaylist:
    case Hit::listRow:
    case Hit::playlistBack:
    case Hit::playPlaylist:
    case Hit::play:
    case Hit::rewind:
    case Hit::loopButton:
    case Hit::stemMute:
    case Hit::stemSolo:
    case Hit::pitchDown:
    case Hit::pitchUp:
    case Hit::emptyImport:
      break;
  }
  (void) song;
}

void StudioView::mouseDrag(const juce::MouseEvent& e)
{
  auto valueAt = [&](juce::Rectangle<float> r) { return juce::jlimit(0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth()); };
  if (mPressed.kind == Hit::wave && e.getDistanceFromDragStart() > 5)
  {
    mDraggedWave = true;
    const double len = mPlayer.length();
    const double a = mDragStart01 * len, b = valueAt(mWaveArea) * len;
    changeMix([a, b](auto& m) {
      m.loopStart = std::min(a, b);
      m.loopEnd = std::max(a, b);
      m.loop = true;
    }, false);
  }
  else if (mPressed.kind == Hit::stemVolume || mPressed.kind == Hit::speed || mPressed.kind == Hit::volume)
    mouseDown(e.withNewPosition(e.position)); // mesmo cálculo do clique
}

void StudioView::mouseUp(const juce::MouseEvent& e)
{
  const auto h = hitAt(e.position);
  const auto pressed = mPressed;
  mPressed = {};
  if (e.mods.isPopupMenu())
    return;
  const auto* song = current();
  // Arrastes: grava a mistura ao soltar.
  if (pressed.kind == Hit::stemVolume || pressed.kind == Hit::speed || (pressed.kind == Hit::wave && mDraggedWave))
  {
    changeMix([](auto&) {}, true);
    return;
  }
  if (!(h == pressed))
    return;
  switch (h.kind)
  {
    case Hit::importButton:
    case Hit::emptyImport:
      chooseFile();
      break;
    case Hit::cancelButton:
      mSeparation.cancel();
      break;
    case Hit::newPlaylist:
    {
      const int index = mLibrary.addPlaylist(tr("Playlist %1").replace("%1", juce::String(static_cast<int>(mLibrary.playlists().size()) + 1)));
      mOpenPlaylist = index;
      resized();
      repaint();
      showPlaylistMenu(index);
      break;
    }
    case Hit::playlistBack:
      mOpenPlaylist = -1;
      mScroll = 0;
      resized();
      repaint();
      break;
    case Hit::playPlaylist:
    {
      const auto& p = mLibrary.playlists()[static_cast<size_t>(mOpenPlaylist)];
      if (!p.songs.isEmpty())
      {
        mPlayingPlaylist = mOpenPlaylist;
        openSong(p.songs[0], true);
      }
      break;
    }
    case Hit::listRow:
    {
      const auto ids = rowIds();
      if (mTab == 1 && mOpenPlaylist < 0)
      {
        mOpenPlaylist = ids[h.index].getIntValue();
        mScroll = 0;
        resized();
        repaint();
      }
      else
      {
        mPlayingPlaylist = mTab == 1 ? mOpenPlaylist : -1;
        openSong(ids[h.index], false);
      }
      break;
    }
    case Hit::play:
      mPlayer.setPlaying(!mPlayer.playing());
      break;
    case Hit::rewind:
      if (song != nullptr)
        mPlayer.seek(song->mix.loop ? song->mix.loopStart : 0.0);
      break;
    case Hit::loopButton:
      changeMix([len = mPlayer.length(), pos = mPlayer.position()](auto& m) {
        m.loop = !m.loop;
        // Sem trecho marcado: 8 s a partir de onde está.
        if (m.loop && m.loopEnd <= m.loopStart + 0.25)
        {
          m.loopStart = pos;
          m.loopEnd = std::min(len, pos + 8.0);
        }
      }, true);
      break;
    case Hit::wave:
      mPlayer.seek(mDragStart01 * mPlayer.length());
      break;
    case Hit::stemMute:
      changeMix([s = h.index](auto& m) { m.mute[static_cast<size_t>(s)] = !m.mute[static_cast<size_t>(s)]; }, true);
      break;
    case Hit::stemSolo:
      changeMix([s = h.index](auto& m) { m.solo[static_cast<size_t>(s)] = !m.solo[static_cast<size_t>(s)]; }, true);
      break;
    case Hit::pitchDown:
      changeMix([](auto& m) { m.semitones = juce::jmax(-6, m.semitones - 1); }, true);
      break;
    case Hit::pitchUp:
      changeMix([](auto& m) { m.semitones = juce::jmin(6, m.semitones + 1); }, true);
      break;
    case Hit::none:
    case Hit::tab:
    case Hit::stemVolume:
    case Hit::speed:
    case Hit::volume:
      break;
  }
  repaint();
}

void StudioView::mouseDoubleClick(const juce::MouseEvent& e)
{
  const auto h = hitAt(e.position);
  if (h.kind == Hit::listRow && !(mTab == 1 && mOpenPlaylist < 0))
    openSong(rowIds()[h.index], true);
  else if (h.kind == Hit::stemVolume)
    changeMix([s = h.index](auto& m) { m.volume[static_cast<size_t>(s)] = 1.0f; }, true);
  else if (h.kind == Hit::speed)
    changeMix([](auto& m) { m.speed = 1.0; }, true);
}

void StudioView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
  if (!mList.contains(e.position))
    return;
  const int visible = static_cast<int>(mList.getHeight() / kRowHeight);
  const int maxScroll = juce::jmax(0, rowIds().size() - visible);
  mScroll = juce::jlimit(0, maxScroll, mScroll - (w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0)));
  repaint();
}

bool StudioView::keyPressed(const juce::KeyPress& key)
{
  if (key == juce::KeyPress::spaceKey && current() != nullptr)
  {
    mPlayer.setPlaying(!mPlayer.playing());
    repaint();
    return true;
  }
  return false;
}

bool StudioView::isInterestedInFileDrag(const juce::StringArray& files)
{
  return files.size() == 1 && juce::File(files[0]).hasFileExtension(nexus::studio::Library::importPattern().replace("*", ""));
}

void StudioView::filesDropped(const juce::StringArray& files, int, int)
{
  if (!files.isEmpty())
    importFile(juce::File(files[0]));
}

// ------------------------------------------------------------------ ações

void StudioView::chooseFile()
{
  mChooser = std::make_unique<juce::FileChooser>(tr("Importar música"), juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                                 nexus::studio::Library::importPattern());
  mChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [safe = juce::Component::SafePointer<StudioView>(this)](const juce::FileChooser& fc) {
                          if (safe != nullptr && fc.getResult().existsAsFile())
                            safe->importFile(fc.getResult());
                        });
}

void StudioView::importFile(const juce::File& file)
{
  if (mSeparation.busy())
  {
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, tr("Estúdio"),
                                           tr("Espere a música que está sendo separada terminar."));
    return;
  }
  mSeparation.start(file, [safe = juce::Component::SafePointer<StudioView>(this)](const juce::String& id, const juce::String& error) {
    if (safe == nullptr)
      return;
    safe->mLibrary.rescan();
    safe->resized();
    if (error.isNotEmpty())
    {
      if (error != "cancelado")
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, tr("Estúdio"), error);
    }
    else
    {
      safe->mTab = 0;
      safe->mTabs.setSelected(0);
      safe->openSong(id, false);
    }
    safe->repaint();
  });
  resized();
  repaint();
}

void StudioView::openSong(const juce::String& id, bool play)
{
  const auto* song = mLibrary.find(id);
  if (song == nullptr)
    return;
  if (mPlayer.songId() != id)
  {
    if (!mPlayer.open(*song))
    {
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, tr("Estúdio"),
                                             tr("Não foi possível abrir as faixas desta música."));
      return;
    }
    mPlayer.setVolume(mMusicVolume * mMusicVolume * 1.25f);
  }
  if (play)
    mPlayer.setPlaying(true);
  repaint();
}

void StudioView::playNextInPlaylist()
{
  if (mPlayingPlaylist < 0 || mPlayingPlaylist >= static_cast<int>(mLibrary.playlists().size()))
    return;
  const auto& songs = mLibrary.playlists()[static_cast<size_t>(mPlayingPlaylist)].songs;
  const int at = songs.indexOf(mPlayer.songId());
  if (at >= 0 && at + 1 < songs.size())
    openSong(songs[at + 1], true);
  else
    mPlayingPlaylist = -1;
}

void StudioView::showSongMenu(const juce::String& id)
{
  juce::PopupMenu menu, add;
  for (size_t i = 0; i < mLibrary.playlists().size(); ++i)
    add.addItem(mLibrary.playlists()[i].name, [this, i, id] {
      mLibrary.addToPlaylist(static_cast<int>(i), id);
      repaint();
    });
  add.addSeparator();
  add.addItem(tr("Nova playlist..."), [this, id] {
    const int index = mLibrary.addPlaylist(tr("Playlist %1").replace("%1", juce::String(static_cast<int>(mLibrary.playlists().size()) + 1)));
    mLibrary.addToPlaylist(index, id);
    repaint();
  });
  menu.addItem(tr("Tocar"), [this, id] { openSong(id, true); });
  menu.addSubMenu(tr("Adicionar à playlist"), add);
  if (mTab == 1 && mOpenPlaylist >= 0)
  {
    const int pl = mOpenPlaylist;
    const int pos = mLibrary.playlists()[static_cast<size_t>(pl)].songs.indexOf(id);
    menu.addSeparator();
    menu.addItem(tr("Subir"), pos > 0, false, [this, pl, pos] { mLibrary.moveInPlaylist(pl, pos, pos - 1); repaint(); });
    menu.addItem(tr("Descer"), pos + 1 < mLibrary.playlists()[static_cast<size_t>(pl)].songs.size(), false,
                 [this, pl, pos] { mLibrary.moveInPlaylist(pl, pos, pos + 1); repaint(); });
    menu.addItem(tr("Tirar da playlist"), [this, pl, pos] { mLibrary.removeFromPlaylist(pl, pos); repaint(); });
  }
  menu.addSeparator();
  menu.addItem(tr("Mostrar as faixas no computador"), [this, id] {
    if (const auto* s = mLibrary.find(id))
      s->folder.revealToUser();
  });
  menu.addItem(tr("Apagar a música"), [this, id] {
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, tr("Apagar a música"),
                                       tr("As faixas separadas vão para a lixeira. O arquivo original não é mexido."),
                                       tr("Apagar"), tr("Cancelar"), this,
                                       juce::ModalCallbackFunction::create([safe = juce::Component::SafePointer<StudioView>(this), id](int ok) {
                                         if (safe == nullptr || ok == 0)
                                           return;
                                         if (safe->mPlayer.songId() == id)
                                           safe->mPlayer.close();
                                         safe->mLibrary.remove(id);
                                         safe->repaint();
                                       }));
  });
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition());
}

void StudioView::showPlaylistMenu(int index)
{
  juce::PopupMenu menu;
  menu.addItem(tr("Renomear..."), [this, index] {
    auto* w = new juce::AlertWindow(tr("Renomear a playlist"), {}, juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor("nome", mLibrary.playlists()[static_cast<size_t>(index)].name);
    w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton(tr("Cancelar"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->enterModalState(true, juce::ModalCallbackFunction::create([safe = juce::Component::SafePointer<StudioView>(this), w, index](int r) {
      if (safe != nullptr && r == 1)
      {
        safe->mLibrary.renamePlaylist(index, w->getTextEditorContents("nome"));
        safe->repaint();
      }
    }), true);
  });
  menu.addItem(tr("Apagar a playlist"), [this, index] {
    mLibrary.removePlaylist(index);
    if (mOpenPlaylist == index)
      mOpenPlaylist = -1;
    if (mPlayingPlaylist == index)
      mPlayingPlaylist = -1;
    resized();
    repaint();
  });
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition());
}

void StudioView::timerCallback()
{
  const bool busy = mSeparation.busy();
  if (busy != mWasBusy)
  {
    // A separação terminou com a tela aberta de novo: atualiza a lista.
    if (!busy)
      mLibrary.rescan();
    mWasBusy = busy;
    resized();
  }
  bool changed = busy || mPlayer.playing();
  for (int s = 0; s < kStems; ++s)
  {
    const float peak = mPlayer.stemLevel(s);
    auto& l = mLevels[static_cast<size_t>(s)];
    const float next = std::max(peak, l * 0.85f);
    changed = changed || std::abs(next - l) > 0.002f;
    l = next;
  }
  if (mPlayer.takeFinished())
  {
    playNextInPlaylist();
    changed = true;
  }
  if (changed)
    repaint();
}
} // namespace nexus::ui
