/* ZenOS hero — interactive 3D hardware diagram (Three.js r160, vendored, no CDN at runtime).
   Style: isometric engineering diagram — central QFP chip, labelled peripheral modules,
   glowing traces, floating RTOS layer stack. Mouse/touch steers the camera smoothly. */
(function () {
  'use strict';
  var hero = document.getElementById('hero');
  var canvas = document.getElementById('hero-canvas');
  if (!hero || !canvas) return;

  var reduceMotion = window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;

  function noScene() {
    hero.classList.remove('scene-ready');
    hero.classList.add('no-scene');
  }

  import('./vendor/three.module.min.js').then(function (THREE) {
    try { boot(THREE); } catch (err) { console.warn('[ZenOS scene]', err); noScene(); }
  }).catch(function () { noScene(); });

  function boot(THREE) {
    /* ---------- renderer ---------- */
    var renderer;
    try {
      renderer = new THREE.WebGLRenderer({ canvas: canvas, antialias: true, alpha: false, powerPreference: 'high-performance' });
    } catch (e) { noScene(); return; }
    renderer.setClearColor(0x081527, 1);
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.3;

    var scene = new THREE.Scene();
    scene.fog = new THREE.Fog(0x081527, 130, 320);

    /* background: vertical navy gradient with faint circuit haze */
    scene.background = gradientTexture(THREE, [
      ['#0a1c33', 0], ['#081728', 0.55], ['#050e1a', 1]
    ]);

    var camera = new THREE.PerspectiveCamera(32, 1, 0.5, 400);

    /* ---------- lights ---------- */
    scene.add(new THREE.HemisphereLight(0x4a7ab0, 0x081322, 1.25));
    var sun = new THREE.DirectionalLight(0xffffff, 1.75);
    sun.position.set(26, 44, 22);
    sun.castShadow = true;
    sun.shadow.mapSize.set(2048, 2048);
    var sc = sun.shadow.camera;
    sc.left = -46; sc.right = 46; sc.top = 46; sc.bottom = -46; sc.near = 8; sc.far = 140;
    scene.add(sun);
    var cyan = new THREE.PointLight(0x37d5ff, 420, 95, 2);
    cyan.position.set(0, 7, 2);
    scene.add(cyan);
    var rim = new THREE.DirectionalLight(0x4aa8ff, 0.75);
    rim.position.set(-30, 16, -26);
    scene.add(rim);

    /* ---------- texture helpers ---------- */
    function gradientTexture(THREE, stops) {
      var c = document.createElement('canvas'); c.width = 4; c.height = 256;
      var x = c.getContext('2d');
      var g = x.createLinearGradient(0, 0, 0, 256);
      stops.forEach(function (s) { g.addColorStop(s[1], s[0]); });
      x.fillStyle = g; x.fillRect(0, 0, 4, 256);
      var t = new THREE.CanvasTexture(c);
      t.colorSpace = THREE.SRGBColorSpace;
      return t;
    }

    function labelTexture(text, opt) {
      opt = opt || {};
      var fs = opt.fs || 72;
      var pad = 26;
      var c = document.createElement('canvas');
      var x = c.getContext('2d');
      var font = (opt.weight || 700) + ' ' + fs + 'px Inter, Segoe UI, Arial, sans-serif';
      x.font = font;
      var lines = String(text).split('\n');
      var w = 0;
      lines.forEach(function (l) { w = Math.max(w, x.measureText(l).width); });
      c.width = Math.ceil(w + pad * 2);
      c.height = Math.ceil(lines.length * fs * 1.22 + pad * 2);
      x = c.getContext('2d');
      x.font = font;
      x.textAlign = 'center';
      x.textBaseline = 'middle';
      if (opt.bg) {
        roundRect(x, 1, 1, c.width - 2, c.height - 2, 16);
        x.fillStyle = opt.bg;
        x.fill();
        x.strokeStyle = opt.border || 'rgba(96,214,255,.55)';
        x.lineWidth = 3;
        x.stroke();
      }
      if (opt.glow !== false) {
        x.shadowColor = opt.color || '#dff4ff';
        x.shadowBlur = opt.glowBlur || 14;
      }
      x.fillStyle = opt.color || '#e8f6ff';
      lines.forEach(function (l, i) {
        x.fillText(l, c.width / 2, pad + fs * 0.61 + i * fs * 1.22);
      });
      var t = new THREE.CanvasTexture(c);
      t.colorSpace = THREE.SRGBColorSpace;
      t.anisotropy = renderer.capabilities.getMaxAnisotropy();
      return { tex: t, aspect: c.width / c.height };
    }

    function roundRect(x, a, b, w, h, r) {
      x.beginPath();
      x.moveTo(a + r, b);
      x.arcTo(a + w, b, a + w, b + h, r);
      x.arcTo(a + w, b + h, a, b + h, r);
      x.arcTo(a, b + h, a, b, r);
      x.arcTo(a, b, a + w, b, r);
      x.closePath();
    }

    /* upright label plane, faces +z (camera side) */
    function makeLabel(THREE, text, height, opt) {
      var L = labelTexture(text, opt);
      var m = new THREE.Mesh(
        new THREE.PlaneGeometry(height * L.aspect, height),
        new THREE.MeshBasicMaterial({ map: L.tex, transparent: true, depthWrite: false, side: THREE.DoubleSide, fog: false })
      );
      m.renderOrder = 5;
      return m;
    }

    /* ---------- PCB board ---------- */
    function boardTexture(THREE) {
      var W = 1600, H = 1000;
      var c = document.createElement('canvas'); c.width = W; c.height = H;
      var x = c.getContext('2d');
      x.fillStyle = '#112942';
      x.fillRect(0, 0, W, H);
      var seed = 7;
      function rnd() { seed = (seed * 16807) % 2147483647; return seed / 2147483647; }
      /* solder mask grid */
      x.strokeStyle = 'rgba(90,160,210,.05)';
      x.lineWidth = 1;
      for (var i = 0; i <= W; i += 40) { x.beginPath(); x.moveTo(i, 0); x.lineTo(i, H); x.stroke(); }
      for (var j = 0; j <= H; j += 40) { x.beginPath(); x.moveTo(0, j); x.lineTo(W, j); x.stroke(); }
      /* manhattan traces */
      for (var t = 0; t < 90; t++) {
        var px = rnd() * W, py = rnd() * H;
        x.strokeStyle = 'rgba(70,170,230,' + (0.14 + rnd() * 0.22).toFixed(2) + ')';
        x.lineWidth = rnd() < 0.25 ? 4 : 2;
        x.beginPath();
        x.moveTo(px, py);
        for (var s = 0; s < 5; s++) {
          if (rnd() < 0.5) px += (rnd() - 0.5) * 320; else py += (rnd() - 0.5) * 240;
          x.lineTo(px, py);
        }
        x.stroke();
        /* via at end */
        x.fillStyle = 'rgba(140,225,255,.5)';
        x.beginPath(); x.arc(px, py, 4.5, 0, 6.284); x.fill();
        x.fillStyle = 'rgba(11,30,51,1)';
        x.beginPath(); x.arc(px, py, 2, 0, 6.284); x.fill();
      }
      /* pads clusters */
      for (var p = 0; p < 26; p++) {
        var cx = rnd() * W, cy = rnd() * H, n = 3 + (rnd() * 5) | 0;
        for (var k = 0; k < n; k++) {
          x.fillStyle = 'rgba(120,210,255,.34)';
          x.beginPath(); x.arc(cx + k * 14 - n * 7, cy, 4, 0, 6.284); x.fill();
        }
      }
      /* silkscreen frame */
      x.strokeStyle = 'rgba(190,230,255,.20)';
      x.lineWidth = 3;
      x.strokeRect(24, 24, W - 48, H - 48);
      var t2 = new THREE.CanvasTexture(c);
      t2.colorSpace = THREE.SRGBColorSpace;
      t2.anisotropy = renderer.capabilities.getMaxAnisotropy();
      return t2;
    }

    function radialTexture(THREE, inner, outer) {
      var c = document.createElement('canvas'); c.width = c.height = 256;
      var x = c.getContext('2d');
      var g = x.createRadialGradient(128, 128, 4, 128, 128, 126);
      g.addColorStop(0, inner);
      g.addColorStop(1, outer);
      x.fillStyle = g;
      x.fillRect(0, 0, 256, 256);
      var t = new THREE.CanvasTexture(c);
      t.colorSpace = THREE.SRGBColorSpace;
      return t;
    }

    /* ---------- materials ---------- */
    var matBoard = new THREE.MeshStandardMaterial({ map: boardTexture(THREE), roughness: 0.78, metalness: 0.18 });
    var matChipBody = new THREE.MeshStandardMaterial({ color: 0x1d2530, roughness: 0.45, metalness: 0.4 });
    var matPin = new THREE.MeshStandardMaterial({ color: 0xc9d4de, roughness: 0.32, metalness: 0.85 });
    var matModule = new THREE.MeshStandardMaterial({ color: 0x243447, roughness: 0.5, metalness: 0.45 });
    var matModuleB = new THREE.MeshStandardMaterial({ color: 0x2c3f56, roughness: 0.6, metalness: 0.35 });
    var matMetal = new THREE.MeshStandardMaterial({ color: 0x9fb2c4, roughness: 0.35, metalness: 0.9 });
    var matTrace = new THREE.MeshBasicMaterial({ color: 0x5fe4ff, fog: false });
    var matGlass = new THREE.MeshStandardMaterial({
      color: 0x6fd4ff, roughness: 0.12, metalness: 0.0, transparent: true, opacity: 0.16,
      emissive: 0x0d3c58, emissiveIntensity: 0.7, side: THREE.DoubleSide, depthWrite: false
    });
    var edgeMat = new THREE.LineBasicMaterial({ color: 0x7fe4ff, transparent: true, opacity: 0.85 });

    var diagram = new THREE.Group();
    scene.add(diagram);

    /* ---------- board ---------- */
    var board = new THREE.Mesh(new THREE.BoxGeometry(66, 1.2, 42), matBoard);
    board.position.y = -0.6;
    board.receiveShadow = true;
    diagram.add(board);
    var boardEdge = new THREE.LineSegments(new THREE.EdgesGeometry(board.geometry), new THREE.LineBasicMaterial({ color: 0x2f6f9e, transparent: true, opacity: 0.55 }));
    boardEdge.position.copy(board.position);
    diagram.add(boardEdge);

    /* ground glow under the chip */
    var glow = new THREE.Mesh(
      new THREE.PlaneGeometry(34, 34),
      new THREE.MeshBasicMaterial({ map: radialTexture(THREE, 'rgba(70,210,255,.55)', 'rgba(70,210,255,0)'), transparent: true, blending: THREE.AdditiveBlending, depthWrite: false })
    );
    glow.rotation.x = -Math.PI / 2;
    glow.position.y = 0.06;
    diagram.add(glow);

    /* ---------- central QFP chip ---------- */
    var chip = new THREE.Group();
    diagram.add(chip);
    var body = new THREE.Mesh(new THREE.BoxGeometry(11, 1.7, 11), matChipBody);
    body.position.y = 1.55;
    body.castShadow = true;
    chip.add(body);
    var bodyEdge = new THREE.LineSegments(new THREE.EdgesGeometry(body.geometry), edgeMat);
    bodyEdge.position.copy(body.position);
    chip.add(bodyEdge);
    var notch = new THREE.Mesh(new THREE.CylinderGeometry(0.55, 0.55, 0.16, 20), new THREE.MeshStandardMaterial({ color: 0x2b3440, roughness: 0.4 }));
    notch.position.set(-4.1, 2.44, -4.1);
    chip.add(notch);
    var pinGeo = new THREE.BoxGeometry(0.55, 0.3, 2.3);
    for (var side = 0; side < 4; side++) {
      for (var pi = 0; pi < 13; pi++) {
        var pin = new THREE.Mesh(pinGeo, matPin);
        var off = (pi - 6) * 0.82;
        if (side === 0) { pin.position.set(off, 0.72, 6.4); }
        if (side === 1) { pin.position.set(off, 0.72, -6.4); }
        if (side === 2) { pin.position.set(6.4, 0.72, off); pin.rotation.y = Math.PI / 2; }
        if (side === 3) { pin.position.set(-6.4, 0.72, off); pin.rotation.y = Math.PI / 2; }
        pin.castShadow = true;
        chip.add(pin);
      }
    }
    var chipLabel = makeLabel(THREE, 'ZenOS\nCortex-M', 2.6, { fs: 96, color: '#f2fbff', weight: 800 });
    chipLabel.rotation.x = -Math.PI / 2;
    chipLabel.position.y = 2.42;
    chip.add(chipLabel);
    var chipSub = makeLabel(THREE, 'REAL-TIME KERNEL', 0.62, { fs: 54, color: '#7fe4ff', weight: 800, glowBlur: 8 });
    chipSub.rotation.x = -Math.PI / 2;
    chipSub.position.set(0, 2.42, 3.6);
    chip.add(chipSub);

    /* ---------- peripheral modules ---------- */
    function makeLabelTHREE(text, h, opt) {
      return makeLabel(THREE, text, h, opt || { fs: 66, color: '#e8f6ff', weight: 800 });
    }

    function moduleBox(w, h, d, mat) {
      var g = new THREE.Group();
      var m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat || matModule);
      m.position.y = h / 2;
      m.castShadow = true;
      g.add(m);
      var e = new THREE.LineSegments(new THREE.EdgesGeometry(m.geometry), edgeMat);
      e.position.copy(m.position);
      g.add(e);
      return g;
    }

    function pinRow(g, count, span, y, z) {
      var pg = new THREE.BoxGeometry(0.34, 0.2, 1.1);
      for (var i = 0; i < count; i++) {
        var p = new THREE.Mesh(pg, matPin);
        p.position.set((i - (count - 1) / 2) * span, y, z);
        g.add(p);
      }
    }

    var modules = [];
    function addModule(spec) {
      var g = moduleBox(spec.w, spec.h, spec.d, spec.mat);
      g.position.set(spec.x, 0, spec.z);
      g.rotation.y = spec.ry || 0;
      if (spec.pins) { pinRow(g, spec.pins, 0.62, 0.28, spec.d / 2 + 0.55); pinRow(g, spec.pins, 0.62, 0.28, -spec.d / 2 - 0.55); }
      if (spec.label) {
        var lb = makeLabelTHREE(spec.label, spec.lh || 1.5, spec.lopt);
        lb.position.y = spec.h + (spec.lh || 1.5) * 0.75;
        g.add(lb);
      }
      diagram.add(g);
      modules.push({ g: g, spec: spec });
      return g;
    }

    /* left cluster: communication interfaces */
    var capL = makeLabel(THREE, 'COMMUNICATION\nINTERFACES', 2.3, { fs: 60, color: '#8fe9ff', weight: 800 });
    capL.position.set(-26.5, 7.2, -6);
    diagram.add(capL);
    addModule({ x: -23, z: -13, w: 5.2, h: 1.1, d: 3, pins: 7, label: 'UART' });
    addModule({ x: -23, z: -6.4, w: 5.2, h: 1.1, d: 3, pins: 7, label: 'SPI' });
    addModule({ x: -23, z: 0.2, w: 5.2, h: 1.1, d: 3, pins: 7, label: 'I2C' });
    addModule({ x: -23, z: 6.8, w: 5.2, h: 1.1, d: 3, pins: 7, label: 'CAN' });
    addModule({ x: -23, z: 13.4, w: 5.6, h: 1.4, d: 3.4, label: 'USB' });

    /* right cluster: memory */
    var capR = makeLabel(THREE, 'MEMORY', 2.1, { fs: 64, color: '#8fe9ff', weight: 800 });
    capR.position.set(24.5, 6.4, -10);
    diagram.add(capR);
    addModule({ x: 22, z: -7, w: 7, h: 2.4, d: 5, mat: matModuleB, pins: 9, label: 'NOR Flash' });
    addModule({ x: 23, z: 4.5, w: 7.6, h: 2, d: 5.4, mat: matModuleB, pins: 9, label: 'SRAM' });

    /* front cluster: sensors + actuators */
    var capF = makeLabel(THREE, 'SENSORS & ACTUATORS', 1.9, { fs: 56, color: '#8fe9ff', weight: 800 });
    capF.position.set(0, 5.4, 17.6);
    diagram.add(capF);
    var tSens = addModule({ x: -12, z: 13.5, w: 3.4, h: 1, d: 2.6, label: 'Temperature', lh: 1.2, lopt: { fs: 52, color: '#cfe9fb', weight: 700 } });
    pinRow(tSens, 4, 0.7, 0.2, 1.9);
    pinRow(tSens, 4, 0.7, 0.2, -1.9);
    var ultra = addModule({ x: -3.5, z: 14, w: 5.6, h: 1, d: 3.6, mat: matModuleB, label: 'Ultrasonic', lh: 1.2, lopt: { fs: 52, color: '#cfe9fb', weight: 700 } });
    [-1.3, 1.3].forEach(function (ex) {
      var eye = new THREE.Mesh(new THREE.CylinderGeometry(1.05, 1.05, 1.3, 24), matMetal);
      eye.rotation.x = Math.PI / 2;
      eye.position.set(ex, 1.6, 0.3);
      eye.castShadow = true;
      ultra.add(eye);
    });
    var motor = new THREE.Group();
    motor.position.set(6, 0, 13.5);
    var canM = new THREE.Mesh(new THREE.CylinderGeometry(1.7, 1.7, 5.4, 28), matMetal);
    canM.rotation.z = Math.PI / 2;
    canM.position.y = 1.8;
    canM.castShadow = true;
    motor.add(canM);
    var shaft = new THREE.Mesh(new THREE.CylinderGeometry(0.35, 0.35, 1.6, 14), matPin);
    shaft.rotation.z = Math.PI / 2;
    shaft.position.set(3.4, 1.8, 0);
    motor.add(shaft);
    var motorLabel = makeLabelTHREE('DC Motor', 1.2, { fs: 52, color: '#cfe9fb', weight: 700 });
    motorLabel.position.y = 4.6;
    motor.add(motorLabel);
    diagram.add(motor);
    var servo = addModule({ x: 14.5, z: 13, w: 3.2, h: 4.6, d: 2.2, mat: matModuleB, label: 'Servo', lh: 1.2, lopt: { fs: 52, color: '#cfe9fb', weight: 700 } });
    var horn = new THREE.Mesh(new THREE.CylinderGeometry(0.9, 0.9, 0.24, 20), matMetal);
    horn.position.y = 4.8;
    servo.add(horn);

    /* ---------- floating RTOS layer stack ---------- */
    var stack = new THREE.Group();
    diagram.add(stack);
    var LAYERS = [
      { t: 'Hardware Abstraction Layer (HAL)', s: 'register access \u2022 clocks \u2022 port layer' },
      { t: 'Drivers', s: 'UART \u2022 SPI \u2022 I2C \u2022 timers \u2022 EXTI' },
      { t: 'Real-Time Kernel', s: 'bitmap scheduler \u2022 priority queues' },
      { t: 'Task & Interrupt Management', s: 'static stacks \u2022 ISR nesting \u2022 deadlines' },
      { t: 'Application Software', s: 'user tasks & control loops' }
    ];
    var panels = [];
    LAYERS.forEach(function (L, i) {
      var g = new THREE.Group();
      var baseY = 8.2 + i * 4.4;
      var geo = new THREE.BoxGeometry(25, 3.0, 14.5);
      var mesh = new THREE.Mesh(geo, matGlass);
      g.add(mesh);
      g.add(new THREE.LineSegments(new THREE.EdgesGeometry(geo), edgeMat));
      var main = makeLabel(THREE, L.t, 1.75, { fs: 64, color: '#f4fcff', weight: 800 });
      main.position.set(0, 0.5, 7.4);
      g.add(main);
      var sub = makeLabel(THREE, L.s, 0.65, { fs: 46, color: '#a8e8ff', weight: 600, glowBlur: 6 });
      sub.position.set(0, -0.9, 7.4);
      g.add(sub);
      g.position.set(0, baseY, -1);
      g.userData = { baseY: baseY, idx: i };
      stack.add(g);
      panels.push(g);
    });

    /* light column: hardware \u2192 software link */
    var beam = new THREE.Mesh(
      new THREE.CylinderGeometry(4.5, 7.5, 6.4, 28, 1, true),
      new THREE.MeshBasicMaterial({ color: 0x39c9ff, transparent: true, opacity: 0.07, blending: THREE.AdditiveBlending, side: THREE.DoubleSide, depthWrite: false })
    );
    beam.position.set(0, 5.4, -1);
    diagram.add(beam);

    /* ---------- glowing traces + pulses ---------- */
    var pulses = [];
    function addTrace(fromX, fromZ, fromW, offset) {
      var dx = fromX, dz = fromZ;
      var len = Math.sqrt(dx * dx + dz * dz) || 1;
      var ux = dx / len, uz = dz / len;
      var p1 = new THREE.Vector3(fromX - ux * fromW, 0.22, fromZ - uz * fromW);
      var p2 = new THREE.Vector3(ux * 7.6, 0.22, uz * 7.6);
      var px = -uz * (offset * 0.5), pz = ux * (offset * 0.5);
      var mid1 = new THREE.Vector3((p1.x + p2.x) / 2 + px, 0.22, (p1.z + p2.z) / 2 + pz);
      var curve = new THREE.CubicBezierCurve3(
        p1,
        new THREE.Vector3(p1.x + (p2.x - p1.x) * 0.35 + px, 0.22, p1.z + (p2.z - p1.z) * 0.35 + pz),
        new THREE.Vector3(p1.x + (p2.x - p1.x) * 0.75 + px * 0.6, 0.22, p1.z + (p2.z - p1.z) * 0.75 + pz * 0.6),
        p2
      );
      var tube = new THREE.Mesh(new THREE.TubeGeometry(curve, 24, 0.16, 6, false), matTrace);
      diagram.add(tube);
      var dot = new THREE.Mesh(
        new THREE.SphereGeometry(0.42, 12, 12),
        new THREE.MeshBasicMaterial({ color: 0xa8f0ff, transparent: true, opacity: 0.9, blending: THREE.AdditiveBlending, depthWrite: false, fog: false })
      );
      diagram.add(dot);
      pulses.push({ curve: curve, dot: dot, phase: Math.random() });
    }
    modules.forEach(function (m, i) { addTrace(m.spec.x, m.spec.z, 3.6, (i % 2 ? 3.2 : -3.2)); });
    addTrace(6, 13.5, 3.4, 3.2);   /* DC motor */

    /* ---------- camera rig + pointer interaction ---------- */
    var BASE_YAW = 0.42, BASE_PITCH = 0.52, DIST = 104;
    var tgt = { x: 0, y: 0 }, cur = { x: 0, y: 0 };
    var lastPointer = -1e9;
    var tgtX = -7;

    function pointerFromEvent(e) {
      var r = hero.getBoundingClientRect();
      if (!r.width || !r.height) return;
      tgt.x = Math.max(-1, Math.min(1, ((e.clientX - r.left) / r.width) * 2 - 1));
      tgt.y = Math.max(-1, Math.min(1, ((e.clientY - r.top) / r.height) * 2 - 1));
      lastPointer = performance.now();
    }
    hero.addEventListener('pointermove', pointerFromEvent, { passive: true });
    hero.addEventListener('pointerleave', function () { tgt.x = 0; tgt.y = 0; }, { passive: true });
    hero.addEventListener('touchmove', function (e) { if (e.touches[0]) pointerFromEvent(e.touches[0]); }, { passive: true });
    hero.addEventListener('touchend', function () { tgt.x = 0; tgt.y = 0; }, { passive: true });

    function layout() {
      var w = hero.clientWidth || window.innerWidth;
      var h = hero.clientHeight || window.innerHeight;
      var aspect = w / Math.max(h, 1);
      camera.aspect = aspect;
      tgtX = aspect >= 1.15 ? -7 : 0;
      DIST = aspect >= 1.15 ? 104 : 128;
      renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, w > 1600 ? 1.6 : 2));
      renderer.setSize(w, h, false);
      camera.updateProjectionMatrix();
    }
    layout();
    window.addEventListener('resize', layout, { passive: true });
    if (window.ResizeObserver) new ResizeObserver(layout).observe(hero);

    /* ---------- render loop ---------- */
    var t0 = performance.now();
    var raf = 0, running = false;
    var camPos = new THREE.Vector3(), look = new THREE.Vector3();

    function frame(now) {
      raf = requestAnimationFrame(frame);
      var t = (now - t0) / 1000;
      var ease = reduceMotion ? 1 : 0.06;
      cur.x += (tgt.x - cur.x) * ease;
      cur.y += (tgt.y - cur.y) * ease;
      var idle = (now - lastPointer) > 3500;
      var drift = (!reduceMotion && idle) ? Math.sin(t * 0.35) * 0.05 : 0;

      var yaw = BASE_YAW + cur.x * 0.30 + drift;
      var pitch = Math.max(0.16, Math.min(1.15, BASE_PITCH + cur.y * 0.16));
      look.set(tgtX + cur.x * 3, 7.5 - cur.y * 3, -1);
      camPos.set(
        look.x + Math.sin(yaw) * Math.cos(pitch) * DIST,
        look.y + Math.sin(pitch) * DIST,
        look.z + Math.cos(yaw) * Math.cos(pitch) * DIST
      );
      camera.position.copy(camPos);
      camera.lookAt(look);

      /* depth-proportional parallax on the floating stack */
      for (var i = 0; i < panels.length; i++) {
        var p = panels[i], u = p.userData;
        p.position.x = cur.x * (0.7 + u.idx * 0.85);
        p.position.y = u.baseY + (reduceMotion ? 0 : Math.sin(t * 0.6 + u.idx * 0.9) * 0.28) - cur.y * (0.3 + u.idx * 0.5);
      }

      /* pulses along the traces */
      if (!reduceMotion) {
        for (var k = 0; k < pulses.length; k++) {
          var pu = pulses[k];
          var u2 = (t * 0.22 + pu.phase) % 1;
          var pos = pu.curve.getPoint(u2);
          pu.dot.position.copy(pos);
          var s = 0.75 + Math.sin(u2 * Math.PI) * 0.5;
          pu.dot.scale.set(s, s, s);
        }
      } else if (pulses.length && !pulses[0].placed) {
        for (var k2 = 0; k2 < pulses.length; k2++) pulses[k2].dot.position.copy(pulses[k2].curve.getPoint(0.5));
        pulses[0].placed = true;
      }

      renderer.render(scene, camera);
    }

    function start() { if (!running) { running = true; raf = requestAnimationFrame(frame); } }
    function stop() { if (running) { running = false; cancelAnimationFrame(raf); } }
    document.addEventListener('visibilitychange', function () { document.hidden ? stop() : start(); });
    canvas.addEventListener('webglcontextlost', function (e) { e.preventDefault(); stop(); noScene(); });

    hero.classList.remove('no-scene');
    hero.classList.add('scene-ready');
    start();
    window.__zenosScene = { renderer: renderer, scene: scene, camera: camera, panels: panels };
  }
})();
