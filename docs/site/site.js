// Nexus Modeler — página do projeto: troca de idioma e demonstração do botão de ganho.
(function () {
  "use strict";
  var raiz = document.documentElement;

  // ---------- Idioma (português ou inglês) ----------
  var textos = {
    pt: {
      titulo: "Nexus Modeler",
      descricao: "A qualidade da captura com a facilidade da modelagem. Plugin de guitarra gratuito e de código aberto para Windows, Mac e Linux que toca capturas NAM A2 da TONE3000.",
      demoTitulo: "Botão de ganho com as posições de 0 a 10"
    },
    en: {
      titulo: "Nexus Modeler",
      descricao: "Capture quality with the ease of modeling. A free, open-source guitar plugin for Windows, Mac and Linux that plays NAM A2 captures from TONE3000.",
      demoTitulo: "Gain knob with positions from 0 to 10"
    }
  };

  // Guarda o texto original (português) de alt e aria-label para poder voltar.
  document.querySelectorAll("[data-alt-en]").forEach(function (el) { el.setAttribute("data-alt-pt", el.getAttribute("alt") || ""); });
  document.querySelectorAll("[data-aria-en]").forEach(function (el) { el.setAttribute("data-aria-pt", el.getAttribute("aria-label") || ""); });

  function aplicarIdioma(lingua) {
    raiz.setAttribute("data-lang", lingua);
    raiz.setAttribute("lang", lingua === "en" ? "en" : "pt-BR");
    document.querySelectorAll("[data-alt-en]").forEach(function (el) { el.setAttribute("alt", el.getAttribute("data-alt-" + lingua)); });
    document.querySelectorAll("[data-aria-en]").forEach(function (el) { el.setAttribute("aria-label", el.getAttribute("data-aria-" + lingua)); });
    var meta = document.querySelector('meta[name="description"]');
    if (meta) meta.setAttribute("content", textos[lingua].descricao);
    var demoTitulo = document.getElementById("demo-titulo");
    if (demoTitulo) demoTitulo.textContent = textos[lingua].demoTitulo;
    atualizarDemo();
  }

  function idiomaInicial() {
    var pedido = new URLSearchParams(location.search).get("lang");
    if (pedido === "en" || pedido === "pt") return pedido;
    try {
      var salvo = localStorage.getItem("nexus-idioma");
      if (salvo === "en" || salvo === "pt") return salvo;
    } catch (e) { /* armazenamento bloqueado: segue sem lembrar */ }
    var sistema = (navigator.language || "pt").toLowerCase();
    return sistema.indexOf("pt") === 0 ? "pt" : "en";
  }

  var botaoIdioma = document.getElementById("trocar-idioma");
  if (botaoIdioma) {
    botaoIdioma.addEventListener("click", function () {
      var nova = raiz.getAttribute("data-lang") === "en" ? "pt" : "en";
      aplicarIdioma(nova);
      try { localStorage.setItem("nexus-idioma", nova); } catch (e) { /* sem armazenamento */ }
    });
  }

  // ---------- Demonstração: o ganho escolhe as capturas ----------
  var NS = "http://www.w3.org/2000/svg";
  var faixa = document.getElementById("demo-valor");
  var marcas = document.getElementById("demo-marcas");
  var ponteiro = document.getElementById("demo-ponteiro");
  var trilho = document.getElementById("demo-trilho");
  var leitura = document.getElementById("demo-leitura");
  var modo = "degraus";
  var losangos = [], numeros = [], pontos = [];

  function anguloDe(valor) { return -135 + valor * 27; } // 0 a 10 ocupa 270 graus

  if (marcas && trilho) {
    for (var i = 0; i <= 10; i++) {
      var a = (anguloDe(i) - 90) * Math.PI / 180;
      var ponto = document.createElementNS(NS, "rect");
      var px = 120 + Math.cos(a) * 82, py = 120 + Math.sin(a) * 82;
      ponto.setAttribute("x", px - 4); ponto.setAttribute("y", py - 4);
      ponto.setAttribute("width", 8); ponto.setAttribute("height", 8);
      ponto.setAttribute("transform", "rotate(45 " + px + " " + py + ")");
      ponto.setAttribute("fill", "#2a2d33"); ponto.setAttribute("stroke", "#3d424a");
      marcas.appendChild(ponto); pontos.push(ponto);
      var numero = document.createElementNS(NS, "text");
      numero.setAttribute("x", 120 + Math.cos(a) * 102);
      numero.setAttribute("y", 120 + Math.sin(a) * 102 + 4.5);
      numero.setAttribute("text-anchor", "middle");
      numero.textContent = String(i);
      marcas.appendChild(numero); numeros.push(numero);
      var losango = document.createElement("span");
      trilho.appendChild(losango); losangos.push(losango);
    }
  }

  function numeroBr(v) {
    var lingua = raiz.getAttribute("data-lang");
    var texto = (Math.round(v * 10) / 10).toFixed(1);
    return lingua === "en" ? texto : texto.replace(".", ",");
  }

  function atualizarDemo() {
    if (!faixa) return;
    var valor = parseFloat(faixa.value);
    var baixo = Math.floor(valor), alto = Math.min(10, baixo + 1);
    var peso = valor - baixo; // quanto da captura de cima entra
    var lingua = raiz.getAttribute("data-lang");
    if (ponteiro) ponteiro.setAttribute("transform", "rotate(" + anguloDe(valor) + " 120 120)");

    for (var i = 0; i <= 10; i++) {
      var nivel = 0;
      if (i === baixo) nivel = 1 - peso;
      if (i === alto && peso > 0) nivel = peso;
      var acesa = nivel > 0.001;
      pontos[i].setAttribute("fill", acesa ? "#29b6ff" : "#2a2d33");
      pontos[i].setAttribute("stroke", acesa ? "#5fd0ff" : "#3d424a");
      pontos[i].setAttribute("fill-opacity", acesa ? (0.35 + 0.65 * nivel).toFixed(2) : "1");
      numeros[i].setAttribute("class", acesa ? "acesa" : "");
      losangos[i].className = acesa ? "acesa" : "";
      losangos[i].style.opacity = acesa ? (0.4 + 0.6 * nivel).toFixed(2) : "";
      losangos[i].style.boxShadow = acesa ? "0 0 " + Math.round(14 * nivel) + "px rgba(41,182,255," + (0.8 * nivel).toFixed(2) + ")" : "";
    }

    var html;
    if (peso < 0.001) {
      html = lingua === "en"
        ? "Gain <strong>" + numeroBr(valor) + "</strong>: capture <strong>" + baixo + "</strong> plays alone, exactly as it was recorded."
        : "Ganho <strong>" + numeroBr(valor) + "</strong>: a captura <strong>" + baixo + "</strong> toca sozinha, exatamente como foi gravada.";
    } else {
      var pBaixo = Math.round((1 - peso) * 100), pAlto = 100 - pBaixo;
      html = lingua === "en"
        ? "Gain <strong>" + numeroBr(valor) + "</strong>: captures <strong>" + baixo + "</strong> (" + pBaixo + "%) and <strong>" + alto + "</strong> (" + pAlto + "%) play together, blended smoothly."
        : "Ganho <strong>" + numeroBr(valor) + "</strong>: as capturas <strong>" + baixo + "</strong> (" + pBaixo + "%) e <strong>" + alto + "</strong> (" + pAlto + "%) tocam juntas, misturadas sem emenda.";
    }
    leitura.innerHTML = html;
    faixa.setAttribute("aria-valuetext", (lingua === "en" ? "Gain " : "Ganho ") + numeroBr(valor));
  }

  if (faixa) {
    faixa.addEventListener("input", atualizarDemo);
    document.querySelectorAll("[data-modo]").forEach(function (botao) {
      botao.addEventListener("click", function () {
        modo = botao.getAttribute("data-modo");
        document.querySelectorAll("[data-modo]").forEach(function (b) { b.setAttribute("aria-pressed", b === botao ? "true" : "false"); });
        faixa.step = modo === "degraus" ? "1" : "0.1";
        if (modo === "degraus") faixa.value = Math.round(parseFloat(faixa.value));
        else if (parseFloat(faixa.value) % 1 === 0 && parseFloat(faixa.value) < 10) faixa.value = parseFloat(faixa.value) + 0.5;
        atualizarDemo();
      });
    });
  }

  aplicarIdioma(idiomaInicial());

  // Botões de copiar o comando do terminal.
  document.querySelectorAll("[data-copiar]").forEach(function (botao) {
    botao.addEventListener("click", function () {
      var texto = botao.getAttribute("data-copiar");
      var feito = function () {
        botao.classList.add("copiado");
        var antes = botao.innerHTML;
        botao.innerHTML = '<span class="pt">Copiado</span><span class="en" lang="en">Copied</span>';
        setTimeout(function () { botao.classList.remove("copiado"); botao.innerHTML = antes; }, 1600);
      };
      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(texto).then(feito, function () {});
      } else {
        var campo = document.createElement("textarea");
        campo.value = texto;
        document.body.appendChild(campo);
        campo.select();
        try { document.execCommand("copy"); feito(); } catch (e) {}
        document.body.removeChild(campo);
      }
    });
  });

  // Botões de baixar: apontam direto para o instalador do último lançamento.
  // Sem lançamento (ou sem rede), ficam na página de lançamentos do GitHub.
  var instaladores = {
    "windows": function (n) { return /-windows-instalador\.exe$/i.test(n); },
    "mac": function (n) { return /\.dmg$/i.test(n); },
    "linux-deb": function (n) { return /linux.*\.deb$/i.test(n); },
    "linux": function (n) { return /linux.*\.tar\.gz$/i.test(n); }
  };
  if (window.fetch) {
    fetch("https://api.github.com/repos/disouz4-dev/Nexus-Modeler/releases/latest")
      .then(function (r) {
        // 404: ainda não há versão publicada.
        if (r.status === 404) return { semVersao: true };
        return r.ok ? r.json() : null;
      })
      .then(function (lancamento) {
        if (lancamento && lancamento.semVersao) {
          document.querySelectorAll("[data-instalador]").forEach(function (link) {
            link.classList.add("em-breve");
            link.removeAttribute("href");
            link.setAttribute("aria-disabled", "true");
            if (link.classList.contains("baixar-botao"))
              link.innerHTML = '<span class="pt">Primeira versão em breve</span><span class="en" lang="en">First release coming soon</span>';
            else
              link.style.display = "none";
          });
          return;
        }
        if (!lancamento || !lancamento.assets) return;
        document.querySelectorAll("[data-instalador]").forEach(function (link) {
          var teste = instaladores[link.getAttribute("data-instalador")];
          var arquivo = lancamento.assets.filter(function (a) { return teste && teste(a.name); })[0];
          if (arquivo) link.setAttribute("href", arquivo.browser_download_url);
        });
      })
      .catch(function () {});
  }

  // ---------- Galeria de equipamentos: filtros e foto maximizada ----------
  var filtros = document.querySelectorAll(".filtro");
  filtros.forEach(function (botao) {
    botao.addEventListener("click", function () {
      var escolha = botao.getAttribute("data-filtro");
      filtros.forEach(function (b) { b.setAttribute("aria-pressed", b === botao ? "true" : "false"); });
      document.querySelectorAll(".catalogo-grupo").forEach(function (g) {
        g.hidden = escolha !== "todos" && g.getAttribute("data-grupo") !== escolha;
      });
    });
  });

  var visor = document.getElementById("visor");
  var visorImg = document.getElementById("visor-img");
  var atual = -1;

  // As setas passam só pelas peças que o filtro está mostrando.
  function pecasVisiveis() {
    return Array.prototype.filter.call(document.querySelectorAll(".cat-abrir"), function (b) {
      return !b.closest(".catalogo-grupo[hidden]");
    });
  }

  function mostrar(indice) {
    var pecas = pecasVisiveis();
    if (!pecas.length) return;
    atual = (indice + pecas.length) % pecas.length;
    var peca = pecas[atual];
    var en = raiz.getAttribute("data-lang") === "en";
    var nome = peca.querySelector("h4").textContent;
    var inspiracao = peca.getAttribute("data-inspiracao");
    visorImg.src = peca.getAttribute("data-grande");
    visorImg.width = peca.getAttribute("data-largura");
    visorImg.height = peca.getAttribute("data-altura");
    visorImg.alt = nome;
    document.getElementById("visor-nome").textContent = nome;
    document.getElementById("visor-categoria").textContent = peca.getAttribute(en ? "data-categoria-en" : "data-categoria");
    document.getElementById("visor-inspiracao").textContent = (en ? "inspired by the " : "inspirado no ") + inspiracao;
    document.getElementById("visor-posicao").textContent = (atual + 1) + (en ? " of " : " de ") + pecas.length;
    // Já carrega as vizinhas, para a seta trocar sem espera.
    [atual - 1, atual + 1].forEach(function (i) {
      new Image().src = pecas[(i + pecas.length) % pecas.length].getAttribute("data-grande");
    });
  }

  if (visor && visor.showModal) {
    document.querySelectorAll(".cat-abrir").forEach(function (botao) {
      botao.addEventListener("click", function () {
        mostrar(pecasVisiveis().indexOf(botao));
        visor.showModal();
      });
    });
    visor.addEventListener("click", function (e) {
      var acao = e.target.closest("[data-visor]");
      if (acao) {
        var qual = acao.getAttribute("data-visor");
        if (qual === "fechar") visor.close();
        else mostrar(atual + (qual === "proxima" ? 1 : -1));
      } else if (e.target === visor || e.target.classList.contains("visor-foto")) {
        visor.close();  // clique fora da foto fecha
      }
    });
    visor.addEventListener("keydown", function (e) {
      if (e.key === "ArrowRight") { mostrar(atual + 1); e.preventDefault(); }
      else if (e.key === "ArrowLeft") { mostrar(atual - 1); e.preventDefault(); }
    });
    visor.addEventListener("close", function () {
      // Volta o foco para a peça aberta por último.
      var pecas = pecasVisiveis();
      if (pecas[atual]) pecas[atual].focus();
    });
    // No celular: arrastar para o lado passa a peça.
    var inicioX = null;
    visor.addEventListener("touchstart", function (e) { inicioX = e.touches[0].clientX; }, { passive: true });
    visor.addEventListener("touchend", function (e) {
      if (inicioX === null) return;
      var dx = e.changedTouches[0].clientX - inicioX;
      if (Math.abs(dx) > 50) mostrar(atual + (dx < 0 ? 1 : -1));
      inicioX = null;
    });
  }
})();
