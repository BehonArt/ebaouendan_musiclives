// Customs Studio's bar, added to the Note Editor and the Section Editor (studio.py).
// It follows the page's own builds (the ROM each one makes) and, for the chart on
// screen, tests the custom in the recomp, exports it as the recomp's custom folder,
// or as a BPS patch of the stock ROM to share.
(() => {
  const chart = () => (typeof chartName !== 'undefined' && chartName) || (typeof songName !== 'undefined' && songName) || '';
  const game = () => {
    const c = chart();
    return /^(OM|OS)/.test(c) ? 'OE2' : /^[ms]\d\d/.test(c) ? 'OE1' : 'EBA';
  };
  const NAMES = {
    EBA: ["Breezin'", "Cruisin'", "Sweatin'", 'Hard Rock'],
    OE2: ['Kigaru (easy)', 'Kakan (normal)', 'Gekiretsu (hard)', 'Karei (insane)'],
    OE1: ['Kigaru (easy)', 'Kakan (normal)', 'Gekiretsu (hard)', 'Karei (insane)'],
  };
  const built = {};                                    // chart -> the ROM its last build made
  let buildFailedAt = 0;                               // when the last build failed

  // the page's builds: their ROMs, as they finish
  const realFetch = window.fetch.bind(window);
  window.fetch = async (url, opts) => {
    const isBuild = typeof url === 'string' && /\/api\/(save|section_build|build_all_diffs)$/.test(url);
    let r;
    try {
      r = await realFetch(url, opts);
    } catch (e) {                                      // no answer at all: say so, not "building" for ever
      if (isBuild) { buildFailedAt = Date.now(); say('the build stopped with no answer: its log is in %LOCALAPPDATA%\eba-recomp\studio\studio.log', true); }
      throw e;
    }
    if (isBuild) {
      r.clone().json().then(j => {
        if (j && j.error) { buildFailedAt = Date.now(); say('the build failed: ' + j.error, true); }
        if (j && j.rom) {
          built[chart()] = j.rom;
          realFetch('/studio/api/built', {method: 'POST', body: JSON.stringify({chart: chart(), rom: j.rom})});
          say('built ' + j.rom.split(/[\\/]/).pop() + ': ready to test or export', false);
          refresh();
        }
      }).catch(() => {});
    }
    return r;
  };

  const css = `
  #studio{position:fixed;right:10px;bottom:10px;z-index:9999;background:#1b1f2a;color:#ddd;border:1px solid #e07a1f;
    border-radius:8px;padding:8px 10px;font:12px/1.4 system-ui,sans-serif;box-shadow:0 4px 18px #0008;width:330px}
  #studio h4{margin:0 0 6px;font-size:13px;color:#f0a050;display:flex;justify-content:space-between;cursor:pointer}
  #studio label{display:block;margin:3px 0}
  #studio input[type=text],#studio select{width:100%;box-sizing:border-box;background:#11141b;color:#eee;border:1px solid #444;border-radius:4px;padding:3px}
  #studio button{margin:4px 4px 0 0;background:#e07a1f;color:#111;border:0;border-radius:4px;padding:5px 8px;font-weight:600;cursor:pointer}
  #studio button.alt{background:#3a4152;color:#eee}
  #studio .st{margin-top:6px;min-height:16px;color:#9ec;white-space:pre-wrap}
  #studio .st.err{color:#f88}
  #studio.min .body{display:none}`;
  const box = document.createElement('div');
  box.id = 'studio';
  box.innerHTML = `<style>${css}</style>
    <h4><span>Customs Studio</span><span id="st-min">–</span></h4>
    <div class="body">
      <div id="st-nav">
        <button class="alt" data-go="/">Note Editor</button><button class="alt" data-go="/section_editor">Section Editor</button>
      </div>
      <div id="st-chart" style="color:#aaa"></div>
      <label>Title <input type="text" id="st-title" placeholder="the song's title"></label>
      <label>Artist <input type="text" id="st-artist" placeholder="its artist"></label>
      <div title="The difficulties you charted. Players can choose only these; a custom without all four can't join a full game conversion or fan campaign.">Charted
        <label style="display:inline;margin-right:6px"><input type="checkbox" class="st-ch" value="e" checked> E</label><label style="display:inline;margin-right:6px"><input type="checkbox" class="st-ch" value="n" checked> N</label><label style="display:inline;margin-right:6px"><input type="checkbox" class="st-ch" value="h" checked> H</label><label style="display:inline"><input type="checkbox" class="st-ch" value="x" checked> X</label>
      </div>
      <label>Test on <select id="st-diff"></select></label>
      <label><input type="checkbox" id="st-auto"> Autoplay (watch it played)</label>
      <div>
        <button id="st-test" title="The custom just built, in the recomp's Customs mode, straight into its stage">▶ Test in recomp</button>
        <button id="st-build" class="alt" title="This editor's ROM build, then the test">Build + test</button>
        <button id="st-check" class="alt" title="The recomp plays the custom through headless, every note on time (under a minute): loads, judged, cleared, and how much song is left after the last note">✔ Validate</button>
      </div>
      <div>
        <button id="st-custom" class="alt" title="Adds it to the Customs menu of your games">Export custom folder</button>
        <button id="st-zip" class="alt" title="The custom folder as a .zip, to share (Import custom folder on the other side)">…as .zip</button>
        <button id="st-bps" class="alt" title="A BPS patch of the stock ROM: share the custom without sharing the game">Export patch (.bps)</button>
        <button id="st-open" class="alt" title="The folder your built ROMs and patches land in">Open builds folder</button>
      </div>
      <div class="st" id="st-msg"></div>
    </div>`;
  document.body.appendChild(box);
  const $s = id => document.getElementById(id);
  function say(m, err) { const e = $s('st-msg'); e.textContent = m; e.className = 'st' + (err ? ' err' : ''); }
  $s('st-min').onclick = () => box.classList.toggle('min');

  function refresh() {
    const c = chart(), g = game();
    $s('st-chart').textContent = c ? `${g} · ${c}` + (built[c] ? ' · built' : ' · not built yet') : 'load a chart';
    const sel = $s('st-diff'), keep = sel.value;
    sel.innerHTML = ['e', 'n', 'h', 'x'].map((d, i) => `<option value="${d}">${NAMES[g][i]}</option>`).join('');
    const m = /_([enhx])(?:_|$)/.exec(c);              // the chart's own difficulty by default
    sel.value = keep || (m ? m[1] : 'n');
  }
  setInterval(refresh, 1500);
  refresh();
  realFetch('/studio/api/info', {method: 'POST', body: '{}'}).then(r => r.json()).then(i => {
    if (i.dumps && !i.dumps.length)
      say('No game dumps found, so there are no charts to load. Choose your dumps in the launcher (eba-recomp.exe), then restart the Studio.', true);
  }).catch(() => {});

  async function call(name, extra) {
    const charted = [...document.querySelectorAll('.st-ch:checked')].map(x => x.value).join('');
    const body = Object.assign({chart: chart(), rom: built[chart()], title: $s('st-title').value.trim(),
                                artist: $s('st-artist').value.trim(), difficulties: charted}, extra || {});
    const r = await (await realFetch('/studio/api/' + name, {method: 'POST', body: JSON.stringify(body)})).json();
    if (r.error) { say(r.error, true); return null; }
    return r;
  }
  async function test() {
    say('making the custom and starting the recomp…');
    const r = await call('test', {difficulty: $s('st-diff').value, autoplay: $s('st-auto').checked});
    if (r) say(`the recomp is starting on ${r.stage} (${r.game}): its log ${r.log}`);
  }
  $s('st-test').onclick = test;
  $s('st-check').onclick = async () => {
    say('the recomp is playing the custom through, headless (under a minute)…');
    const r = await call('validate', {difficulty: $s('st-diff').value});
    if (r) say(r.text, r.problems.length > 0);
  };
  $s('st-build').onclick = () => {
    const b = document.getElementById('saveRomBtn') || document.getElementById('btn-build');
    if (!b) { say('no Build ROM button on this page', true); return; }
    const before = built[chart()];
    say('building the ROM…');
    b.click();
    const t0 = Date.now();
    const wait = setInterval(() => {                   // the build's ROM, then the test
      if (buildFailedAt >= t0) clearInterval(wait);    // (it said why)
      else if (built[chart()] && built[chart()] !== before) { clearInterval(wait); test(); }
      else if (Date.now() - t0 > 600000) { clearInterval(wait); say('the build did not finish', true); }
    }, 500);
  };
  $s('st-open').onclick = () => realFetch('/studio/api/open_builds', {method: 'POST', body: JSON.stringify({game: game()})});
  $s('st-custom').onclick = async () => {
    say('exporting…');
    const r = await call('export_custom');
    if (r) say(`${r.tag}: ${r.files} files, over ${r.stage}, in ${r.dir}`);
  };
  $s('st-zip').onclick = async () => {
    say('exporting…');
    const r = await call('export_custom', {zip: true});
    if (r) say(`${r.tag}.zip made: ${r.zip}`);
  };
  $s('st-bps').onclick = async () => {
    say('making the patch (a few seconds)…');
    const r = await call('export_patch');
    if (r) say(`patch made (${(r.size / 1e6).toFixed(1)} MB): ${r.path}`);
  };
})();

// In the app window, the editors' own "open in a new tab" (and these buttons) open another window of it
(function () {
  const browserOpen = window.open;
  window.open = function (u, ...rest) {
    const api = window.pywebview && window.pywebview.api;
    if (api && api.open_window && typeof u === 'string' && u.startsWith('/')) { api.open_window(u); return null; }
    return browserOpen.call(window, u, ...rest);
  };
  document.addEventListener('click', e => {
    const b = e.target.closest && e.target.closest('#st-nav button');
    if (b) location.href = b.dataset.go;
  });
})();
