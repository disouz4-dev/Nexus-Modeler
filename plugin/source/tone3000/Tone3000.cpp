#include "Tone3000.h"

#include <juce_cryptography/juce_cryptography.h>

#include <map>
#include <regex>
#include <set>

#include "BinaryData.h"
#include "nexus/Pack.h"

#ifndef NEXUS_T3K_KEY
  #define NEXUS_T3K_KEY ""
#endif

namespace nexus::t3k
{
namespace
{
const juce::String kOrigin = "https://www.tone3000.com";
constexpr int kFlowTimeoutMs = 5 * 60 * 1000;

juce::String base64url(const void* data, size_t size)
{
  return juce::Base64::toBase64(data, size).replaceCharacter('+', '-').replaceCharacter('/', '_').removeCharacters("=");
}

juce::String randomBase64url(int bytes)
{
  juce::MemoryBlock block(static_cast<size_t>(bytes));
  auto& rng = juce::Random::getSystemRandom();
  for (size_t i = 0; i < block.getSize(); ++i)
    block[i] = static_cast<char>(rng.nextInt(256));
  return base64url(block.getData(), block.getSize());
}

juce::StringPairArray parseQuery(const juce::String& query)
{
  juce::StringPairArray params;
  for (const auto& pair : juce::StringArray::fromTokens(query.trimCharactersAtStart("?"), "&", {}))
    if (pair.isNotEmpty())
      params.set(juce::URL::removeEscapeChars(pair.upToFirstOccurrenceOf("=", false, false).replaceCharacter('+', ' ')),
                 juce::URL::removeEscapeChars(pair.fromFirstOccurrenceOf("=", false, false).replaceCharacter('+', ' ')));
  return params;
}

juce::var recommendedJson()
{
  static const juce::var json = juce::JSON::parse(juce::String::fromUTF8(BinaryData::recomendadas_json, BinaryData::recomendadas_jsonSize));
  return json;
}

Tone toneFromApi(const juce::var& v)
{
  Tone t;
  t.id = static_cast<int>(v["id"]);
  t.title = v["title"].toString().trim();
  t.gear = v["gear"].toString();
  t.models = static_cast<int>(v["a2_models_count"]);
  if (t.models == 0)
    t.models = static_cast<int>(v["models_count"]);
  t.downloads = static_cast<int>(v["downloads_count"]);
  if (v["user"].isObject())
    t.creator = v["user"]["display_name"].toString().isNotEmpty() ? v["user"]["display_name"].toString()
                                                                 : v["user"]["username"].toString();
  return t;
}

// Nome de arquivo seguro, igual em Windows, Mac e Linux.
juce::String safeName(const juce::String& s)
{
  auto n = juce::File::createLegalFileName(s.trim()).replaceCharacters("/\\:", "---");
  return n.isEmpty() ? juce::String("captura") : n.substring(0, 80);
}

// Mensagens que a pessoa vê, em vez dos códigos da TONE3000.
juce::String explain(const juce::String& code)
{
  if (code == "access_denied" || code == "canceled")
    return TRANS(juce::String::fromUTF8("Entrada na TONE3000 cancelada."));
  if (code == "timeout")
    return TRANS(juce::String::fromUTF8("A TONE3000 não respondeu a tempo. Tente de novo."));
  if (code == "state_mismatch")
    return TRANS(juce::String::fromUTF8("Resposta da TONE3000 não confere. Tente de novo."));
  if (code == "rede")
    return TRANS(juce::String::fromUTF8("Sem conexão com a TONE3000. Confira a internet."));
  if (code == "sem_conta")
    return TRANS("Entre na sua conta da TONE3000 primeiro.");
  return TRANS("TONE3000: %1").replace("%1", code);
}
} // namespace

// ================================================================ Recomendadas

std::vector<Tone> recommended(const juce::String& gearPrefix)
{
  std::vector<Tone> out;
  const auto entry = recommendedJson()[juce::Identifier(gearPrefix)];
  if (const auto* tones = entry["tons"].getArray())
    for (const auto& v : *tones)
    {
      Tone t;
      t.id = static_cast<int>(v["id"]);
      t.title = v["titulo"].toString();
      t.creator = v["criador"].toString();
      t.gear = v["tipo"].toString();
      t.models = static_cast<int>(v["capturas"]);
      t.downloads = static_cast<int>(v["downloads"]);
      t.pattern = v["posicao"].toString();
      if (const auto* s = v["serie"].getArray())
        for (const auto& part : *s)
          t.series.add(part.toString());
      t.note = v["nota"].toString();
      out.push_back(t);
    }
  return out;
}

juce::String filterWord(const juce::String& gearPrefix)
{
  return recommendedJson()[juce::Identifier(gearPrefix)]["busca"].toString();
}

// ================================================================ Escolha das capturas

std::vector<Capture> chooseCaptures(const std::vector<std::pair<juce::String, juce::String>>& models, const Tone& tone)
{
  // Posição pelo padrão da recomendada; senão pelas palavras comuns
  // ("Gain 7", "G7", "Drive 3", "D08"), e por fim pela regra do motor.
  static const std::regex generic(R"((?:^|[^a-z])(?:gain|ganho|drive|dist|gn|g|d)[\s_\-=:]*([0-9]+(?:[.,][0-9]+)?))", std::regex::icase);
  struct Found
  {
    double position;
    juce::String group; // o nome sem o número: capturas da mesma série
    size_t index;
  };
  std::vector<Found> found;
  for (size_t i = 0; i < models.size(); ++i)
  {
    const auto& name = models[i].first;
    bool inSeries = true;
    for (const auto& part : tone.series)
      inSeries = inSeries && name.containsIgnoreCase(part);
    if (!inSeries)
      continue;
    const std::string s = name.toStdString();
    std::smatch m;
    bool ok = false;
    double position = 0.0;
    std::string group = s;
    if (tone.pattern.isNotEmpty())
    {
      const std::regex own(tone.pattern.toStdString(), std::regex::icase);
      ok = std::regex_search(s, m, own) && m.size() > 1;
    }
    else
      ok = std::regex_search(s, m, generic);
    if (ok)
    {
      std::string number = m[1].str();
      std::replace(number.begin(), number.end(), ',', '.');
      position = std::stod(number);
    }
    else if (!nexus::positionFromFileName(s, position))
      continue;
    // Série: o nome sem números ("ZW - LOW - G" e "ZW - HIGH - G").
    found.push_back({position, juce::String(group).removeCharacters("0123456789.,").trim(), i});
  }

  // Sem série definida: fica a série com mais posições diferentes.
  std::map<juce::String, std::set<double>> groups;
  for (const auto& f : found)
    groups[f.group].insert(f.position);
  juce::String best;
  size_t bestCount = 0;
  for (const auto& [g, positions] : groups)
    if (positions.size() > bestCount)
    {
      bestCount = positions.size();
      best = g;
    }
  // Com receita, a série já foi filtrada. Sem receita, vale a série com mais
  // posições, a não ser que cada nome seja uma série de uma captura só.
  const bool oneGroup = tone.series.size() > 0 || tone.pattern.isNotEmpty() || bestCount <= 1;

  std::vector<Capture> out;
  std::set<double> used;
  for (const auto& f : found)
  {
    if (!oneGroup && f.group != best)
      continue;
    if (!used.insert(f.position).second)
      continue; // uma captura por posição
    out.push_back({f.position, models[f.index].first, models[f.index].second});
  }
  std::sort(out.begin(), out.end(), [](const Capture& a, const Capture& b) { return a.position < b.position; });
  return out;
}

// ================================================================ Servidor local do login

// Escuta uma única vez em 127.0.0.1 a volta do navegador (RFC 8252) e
// responde com uma página dizendo que já pode fechar a janela.
class Connection::Loopback : private juce::Thread
{
public:
  Loopback() : juce::Thread("nexus-tone3000-login") {}
  ~Loopback() override { stop(); }

  bool start()
  {
    mListener = std::make_unique<juce::StreamingSocket>();
    if (!mListener->createListener(0, "127.0.0.1"))
      return false;
    mPort = mListener->getBoundPort();
    startThread();
    return true;
  }

  void stop()
  {
    signalThreadShouldExit();
    if (mListener)
      mListener->close();
    stopThread(1000);
    mListener.reset();
  }

  juce::String redirectUri() const { return "http://localhost:" + juce::String(mPort) + "/"; }
  std::function<void(const juce::String&)> onQuery;

private:
  void run() override
  {
    while (!threadShouldExit() && mListener != nullptr)
    {
      if (mListener->waitUntilReady(true, 250) != 1)
        continue;
      std::unique_ptr<juce::StreamingSocket> client(mListener->waitForNextConnection());
      if (client != nullptr && serve(*client))
        return;
    }
  }

  bool serve(juce::StreamingSocket& client)
  {
    juce::MemoryBlock data;
    char chunk[1024];
    const auto until = juce::Time::getMillisecondCounter() + 3000;
    while (juce::Time::getMillisecondCounter() < until && !threadShouldExit())
    {
      if (client.waitUntilReady(true, 100) != 1)
        continue;
      const int n = client.read(chunk, sizeof(chunk), false);
      if (n <= 0)
        break;
      data.append(chunk, static_cast<size_t>(n));
      if (data.toString().contains("\r\n\r\n"))
        break;
    }
    const auto line = data.toString().upToFirstOccurrenceOf("\r\n", false, false);
    const auto target = line.fromFirstOccurrenceOf(" ", false, false).upToFirstOccurrenceOf(" ", false, false);
    if (!line.startsWith("GET ") || target.upToFirstOccurrenceOf("?", false, false) != "/")
    {
      const juce::String reply = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      client.write(reply.toRawUTF8(), static_cast<int>(reply.getNumBytesAsUTF8()));
      return false;
    }
    const auto query = target.fromFirstOccurrenceOf("?", false, false);
    const bool ok = parseQuery(query).containsKey("code");
    juce::String body;
    body << "<!doctype html><html><head><meta charset=\"utf-8\"><title>Nexus Modeler</title><style>"
            "body{margin:0;min-height:100vh;background:#0c0c0e;color:#e8e8ea;font:16px/1.5 -apple-system,Segoe UI,Arial,sans-serif;"
            "display:flex;align-items:center;justify-content:center;text-align:center}"
            ".c{width:340px;padding:32px;border:1px solid #2a2c31;border-radius:14px}"
            "h1{margin:0 0 12px;font-size:20px}b{color:#29b6ff}</style></head><body><div class=\"c\">"
         << "<h1>" << (ok ? "Nexus Modeler + TONE3000" : "Nexus Modeler") << "</h1><p>"
         << (ok ? juce::String::fromUTF8("Conta conectada. Pode fechar esta janela e voltar ao <b>Nexus Modeler</b>.")
                : juce::String::fromUTF8("A entrada não terminou. Volte ao <b>Nexus Modeler</b> e tente de novo."))
         << "</p></div><script>window.close()</script></body></html>";
    juce::String reply;
    reply << "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " << static_cast<int>(body.getNumBytesAsUTF8())
          << "\r\nConnection: close\r\n\r\n" << body;
    client.write(reply.toRawUTF8(), static_cast<int>(reply.getNumBytesAsUTF8()));
    juce::MessageManager::callAsync([fn = onQuery, query] { if (fn) fn(query); });
    return true;
  }

  std::unique_ptr<juce::StreamingSocket> mListener;
  int mPort = 0;
};

// ================================================================ Conexão

Connection::Connection()
{
  mKey = NEXUS_T3K_KEY;
  // Também pode vir do ambiente, para testar uma chave sem recompilar.
  if (const auto env = juce::SystemStats::getEnvironmentVariable("NEXUS_T3K_KEY", {}); env.isNotEmpty())
    mKey = env;
  juce::PropertiesFile::Options o;
  o.applicationName = "tone3000";
  o.folderName = "Nexus Modeler";
  o.filenameSuffix = "settings";
  o.osxLibrarySubFolder = "Application Support";
  mPrefs = std::make_unique<juce::PropertiesFile>(o);
  if (mKey.isEmpty())
    mKey = mPrefs->getValue("chave_publica");
}

Connection::~Connection()
{
  stopTimer();
  if (mLoopback)
    mLoopback->stop();
  mPool.removeAllJobs(true, 10000);
}

juce::File Connection::packsFolder()
{
  return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
    .getChildFile("Application Support")
#endif
    .getChildFile("Nexus Modeler")
    .getChildFile("Capturas TONE3000");
}

void Connection::post(std::function<void()> fn)
{
  juce::MessageManager::callAsync([self = juce::WeakReference<Connection>(this), fn = std::move(fn)] {
    if (self != nullptr)
      fn();
  });
}

Connection::Tokens Connection::loadTokens() const
{
  const juce::ScopedLock lock(mPrefsLock);
  mPrefs->reload();
  return {mPrefs->getValue("acesso"), mPrefs->getValue("renovacao"), mPrefs->getValue("expira").getLargeIntValue()};
}

void Connection::saveTokens(const Tokens& t)
{
  const juce::ScopedLock lock(mPrefsLock);
  mPrefs->setValue("acesso", t.access);
  mPrefs->setValue("renovacao", t.refresh);
  mPrefs->setValue("expira", juce::String(t.expiresAt));
  mPrefs->saveIfNeeded();
}

bool Connection::signedIn() const
{
  return loadTokens().refresh.isNotEmpty();
}

juce::String Connection::userName() const
{
  const juce::ScopedLock lock(mPrefsLock);
  return mPrefs->getValue("usuario");
}

void Connection::signOut()
{
  const juce::ScopedLock lock(mPrefsLock);
  for (auto* k : {"acesso", "renovacao", "expira", "usuario"})
    mPrefs->removeValue(k);
  mPrefs->saveIfNeeded();
}

bool Connection::exchange(const juce::StringPairArray& form, juce::String& error)
{
  auto url = juce::URL(kOrigin + "/api/v1/oauth/token");
  for (const auto& k : form.getAllKeys())
    url = url.withParameter(k, form[k]);
  int status = 0;
  auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                                        .withExtraHeaders("Content-Type: application/x-www-form-urlencoded\r\nAccept: application/json")
                                        .withConnectionTimeoutMs(20000)
                                        .withStatusCode(&status));
  if (stream == nullptr)
  {
    error = "rede";
    return false;
  }
  const auto reply = juce::JSON::parse(stream->readEntireStreamAsString());
  if (status / 100 != 2 || reply["access_token"].toString().isEmpty())
  {
    error = reply["error"].toString().isNotEmpty() ? reply["error"].toString() : "token " + juce::String(status);
    return false;
  }
  Tokens t;
  t.access = reply["access_token"].toString();
  t.refresh = reply["refresh_token"].toString();
  t.expiresAt = juce::Time::currentTimeMillis() + static_cast<juce::int64>(static_cast<double>(reply["expires_in"]) * 1000.0);
  saveTokens(t);
  return true;
}

juce::String Connection::accessToken()
{
  auto t = loadTokens();
  if (t.refresh.isEmpty())
    return {};
  if (juce::Time::currentTimeMillis() < t.expiresAt - 60000)
    return t.access;
  juce::StringPairArray form;
  form.set("grant_type", "refresh_token");
  form.set("refresh_token", t.refresh);
  form.set("client_id", mKey);
  juce::String error;
  if (!exchange(form, error))
  {
    // Renovação recusada: outra janela pode ter renovado antes; senão, sai.
    const auto again = loadTokens();
    if (again.refresh != t.refresh && juce::Time::currentTimeMillis() < again.expiresAt - 60000)
      return again.access;
    if (error == "invalid_grant")
      signOut();
    return {};
  }
  return loadTokens().access;
}

juce::var Connection::getJson(const juce::String& path, juce::String& error, const juce::String& token)
{
  int status = 0;
  auto stream = juce::URL(kOrigin + path)
                  .createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                       .withExtraHeaders("Accept: application/json"
                                                         + (token.isNotEmpty() ? "\r\nAuthorization: Bearer " + token : juce::String()))
                                       .withConnectionTimeoutMs(20000)
                                       .withStatusCode(&status));
  if (stream == nullptr)
  {
    error = "rede";
    return {};
  }
  const auto text = stream->readEntireStreamAsString();
  if (status / 100 != 2)
  {
    error = status == 401 ? juce::String("sem_conta") : "HTTP " + juce::String(status);
    return {};
  }
  return juce::JSON::parse(text);
}

// ---------------------------------------------------------------- Telas da TONE3000

void Connection::runFlow(const juce::StringPairArray& extra, std::function<void(int, juce::String)> done)
{
  if (!hasKey())
  {
    done(0, TRANS(juce::String::fromUTF8("Esta compilação não tem a chave da TONE3000.")));
    return;
  }
  if (mLoopback)
    mLoopback->stop();
  mLoopback = std::make_unique<Loopback>();
  if (!mLoopback->start())
  {
    done(0, TRANS(juce::String::fromUTF8("Não consegui abrir a porta local para o login.")));
    return;
  }
  mVerifier = randomBase64url(32);
  mState = randomBase64url(16);
  mRedirect = mLoopback->redirectUri();
  mFlowDone = std::move(done);
  mFlowDeadline = juce::Time::getMillisecondCounter() + kFlowTimeoutMs;
  mLoopback->onQuery = [self = juce::WeakReference<Connection>(this)](const juce::String& q) {
    if (self != nullptr)
      self->finishFlow(q);
  };

  const juce::SHA256 hash(mVerifier.toRawUTF8(), mVerifier.getNumBytesAsUTF8());
  const auto digest = hash.getRawData();
  auto url = juce::URL(kOrigin + "/api/v1/oauth/authorize")
               .withParameter("client_id", mKey)
               .withParameter("redirect_uri", mRedirect)
               .withParameter("response_type", "code")
               .withParameter("code_challenge", base64url(digest.getData(), digest.getSize()))
               .withParameter("code_challenge_method", "S256")
               .withParameter("state", mState);
  for (const auto& k : extra.getAllKeys())
    url = url.withParameter(k, extra[k]);
  url.launchInDefaultBrowser();
  startTimer(1000);
}

void Connection::timerCallback()
{
  if (mFlowDone && juce::Time::getMillisecondCounter() > mFlowDeadline)
    finishFlow("error=timeout&state=" + mState);
}

void Connection::finishFlow(const juce::String& query)
{
  stopTimer();
  if (mLoopback)
    mLoopback->stop();
  auto done = std::move(mFlowDone);
  mFlowDone = nullptr;
  if (!done)
    return;
  const auto p = parseQuery(query);
  if (p["state"] != mState)
    return done(0, explain("state_mismatch"));
  if (p["code"].isEmpty())
    return done(0, explain(p["canceled"] == "true" ? juce::String("canceled") : p["error"].isNotEmpty() ? p["error"] : juce::String("canceled")));

  juce::StringPairArray form;
  form.set("grant_type", "authorization_code");
  form.set("code", p["code"]);
  form.set("code_verifier", mVerifier);
  form.set("redirect_uri", mRedirect);
  form.set("client_id", mKey);
  const int toneId = p["tone_id"].getIntValue();
  mPool.addJob([this, form, toneId, done] {
    juce::String error;
    if (exchange(form, error))
    {
      // Nome da conta, para mostrar "conectado como @fulano".
      juce::String ignore;
      const auto user = getJson("/api/v1/user", ignore, loadTokens().access);
      if (user["username"].toString().isNotEmpty())
      {
        const juce::ScopedLock lock(mPrefsLock);
        mPrefs->setValue("usuario", user["username"].toString());
        mPrefs->saveIfNeeded();
      }
    }
    post([done, toneId, error] { done(toneId, error.isEmpty() ? juce::String() : explain(error)); });
  });
}

void Connection::signIn(std::function<void(juce::String)> done)
{
  runFlow({}, [done](int, juce::String error) { done(error); });
}

// ---------------------------------------------------------------- Listas

void Connection::list(List which, const juce::String& gear, const juce::String& word,
                      std::function<void(std::vector<Tone>, juce::String)> done)
{
  mPool.addJob([this, which, gear, word, done] {
    juce::String error;
    const auto token = accessToken();
    if (token.isEmpty() && which != List::trending)
      return post([done] { done({}, explain("sem_conta")); });
    juce::String path;
    if (which == List::trending)
      path = "/api/v1/tones/trending?gear=" + juce::URL::addEscapeChars(gear, true);
    else
    {
      path = juce::String(which == List::favorites ? "/api/v1/tones/favorited" : "/api/v1/tones/downloaded")
             + "?page=1&page_size=50&gear=" + juce::URL::addEscapeChars(gear, true);
      if (word.isNotEmpty())
        path << "&query=" << juce::URL::addEscapeChars(word, true);
    }
    const auto json = getJson(path, error, token);
    std::vector<Tone> tones;
    if (const auto* data = json["data"].getArray())
      for (const auto& v : *data)
        if (v["format"].toString().isEmpty() || v["format"].toString().equalsIgnoreCase("nam"))
          tones.push_back(toneFromApi(v));
    post([done, tones = std::move(tones), error] { done(tones, error.isEmpty() ? juce::String() : explain(error)); });
  });
}

// ---------------------------------------------------------------- Download

void Connection::download(const Tone& wanted, const juce::String& gearFilter, std::function<void(juce::String)> progress,
                          std::function<void(juce::File, juce::String)> done)
{
  // Plano gratuito: o tom passa pela tela da TONE3000 ("load_tone" com o
  // número, ou "select_tone" para escolher no catálogo). Já conectado, ela
  // confirma e volta sozinha.
  juce::StringPairArray extra;
  if (wanted.id > 0)
  {
    extra.set("prompt", "load_tone");
    extra.set("tone_id", juce::String(wanted.id));
  }
  else
  {
    extra.set("prompt", "select_tone");
    extra.set("gears", gearFilter);
    extra.set("format", "nam");
    extra.set("architecture", "2");
  }
  progress(TRANS(juce::String::fromUTF8("Confirmando na TONE3000...")));
  runFlow(extra, [this, wanted, progress, done](int toneId, juce::String error) {
    if (error.isNotEmpty())
      return done({}, error);
    const int id = toneId > 0 ? toneId : wanted.id;
    if (id <= 0)
      return done({}, explain("canceled"));
    mPool.addJob([this, wanted, id, progress, done] {
      auto say = [this, progress](juce::String s) { post([progress, s] { progress(s); }); };
      auto fail = [this, done](juce::String e) { post([done, e] { done({}, explain(e)); }); };
      juce::String error;
      const auto token = accessToken();
      if (token.isEmpty())
        return fail("sem_conta");

      // Dados do tom (o "load_tone" pode devolver outro, se o original saiu do ar).
      Tone tone = toneFromApi(getJson("/api/v1/tones/" + juce::String(id), error, token));
      if (tone.id == 0)
      {
        tone = wanted;
        tone.id = id;
      }
      if (id == wanted.id)
      {
        tone.pattern = wanted.pattern;
        tone.series = wanted.series;
      }
      say(TRANS("Lendo as capturas de %1...").replace("%1", tone.title));
      const auto models = getJson("/api/v1/models?tone_id=" + juce::String(id) + "&page=1&page_size=300&architecture=2", error, token);
      if (error.isNotEmpty())
        return fail(error);
      std::vector<std::pair<juce::String, juce::String>> list;
      if (const auto* data = models["data"].getArray())
        for (const auto& m : *data)
          list.emplace_back(m["name"].toString(), m["model_url"].toString());
      const auto chosen = chooseCaptures(list, tone);
      if (chosen.empty())
        return fail(TRANS(juce::String::fromUTF8("este tom não tem capturas A2 com a posição do botão no nome")));

      const auto folder = packsFolder().getChildFile(juce::String(id) + " " + safeName(tone.title));
      folder.createDirectory();
      auto* manifest = new juce::DynamicObject();
      auto* captures = new juce::DynamicObject();
      int n = 0;
      for (const auto& c : chosen)
      {
        ++n;
        say(TRANS("Baixando captura %1 de %2...").replace("%1", juce::String(n)).replace("%2", juce::String(static_cast<int>(chosen.size()))));
        const auto file = folder.getChildFile(safeName(c.name) + ".nam");
        if (!file.existsAsFile() || file.getSize() == 0)
        {
          int status = 0;
          auto stream = juce::URL(c.url).createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                             .withExtraHeaders("Authorization: Bearer " + token)
                                                             .withConnectionTimeoutMs(30000)
                                                             .withStatusCode(&status));
          if (stream == nullptr || status / 100 != 2)
            return fail(stream == nullptr ? juce::String("rede") : "HTTP " + juce::String(status));
          juce::TemporaryFile temp(file);
          {
            juce::FileOutputStream out(temp.getFile());
            if (!out.openedOk() || out.writeFromInputStream(*stream, -1) <= 0)
              return fail(TRANS(juce::String::fromUTF8("não consegui gravar a captura no disco")));
          }
          temp.overwriteTargetFileWithTemporary();
        }
        const bool whole = std::abs(c.position - std::round(c.position)) < 1e-6;
        captures->setProperty(whole ? juce::String(juce::roundToInt(c.position)) : juce::String(c.position, 2), file.getFileName());
      }
      manifest->setProperty("nome", tone.title);
      manifest->setProperty("fonte", "TONE3000");
      manifest->setProperty("tone3000_tone_id", id);
      manifest->setProperty("criador", tone.creator);
      manifest->setProperty("endereco", kOrigin + "/tones/" + juce::String(id));
      manifest->setProperty("inclui_caixa", tone.gear == "amp-cab" || tone.gear == "full-rig");
      manifest->setProperty("botao", tone.gear == "pedal" ? "Drive" : "Ganho");
      manifest->setProperty("capturas", juce::var(captures));
      folder.getChildFile("pack.json").replaceWithText(juce::JSON::toString(juce::var(manifest)));
      post([done, folder] { done(folder, {}); });
    });
  });
}
} // namespace nexus::t3k
