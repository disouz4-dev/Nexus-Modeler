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
})();
