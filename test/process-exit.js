'use strict';

var path = require('path');
var spawnSync = require('child_process').spawnSync;
var test = require('./lib/test');

test('MediaStream suite exits naturally after local and remote clone cleanup', function(t) {
  const result = spawnSync(process.execPath, ['--expose-gc', '--test', 'test/mediastream.js'], {
    cwd: path.resolve(__dirname, '..'),
    encoding: 'utf8',
    timeout: 15000
  });
  t.ifError(result.error);
  t.equal(result.signal, null, result.stderr || 'no teardown signal');
  t.equal(result.status, 0, result.stdout + result.stderr);
  t.end();
});

test('closed WebRTC objects exit without a native teardown abort', function(t) {
  var script = [
    'const { RTCPeerConnection } = require(\'./\')',
    'const peer = new RTCPeerConnection()',
    'const channel = peer.createDataChannel(\'exit\')',
    'channel.close()',
    'peer.close()',
    'setTimeout(() => {}, 25)'
  ].join(';');
  var result = spawnSync(process.execPath, ['-e', script], {
    cwd: path.resolve(__dirname, '..'),
    encoding: 'utf8'
  });

  t.equal(result.status, 0, result.stderr || 'process exited cleanly');
  t.end();
});

test('closed stats exit cleanly with retained wrappers, pending requests and garbage collection', function(t) {
  for (const collect of [false, true]) {
    var script = `
      const assert = require('node:assert/strict');
      const { RTCPeerConnection } = require('./');
      (async () => {
        let peer = new RTCPeerConnection();
        let endpoints = peer.addTransceiver('audio');
        const pending = [peer.getStats(), endpoints.sender.getStats(), endpoints.receiver.getStats()];
        peer.close();
        pending.push(peer.getStats(), endpoints.sender.getStats(), endpoints.receiver.getStats());
        if (${collect}) {
          endpoints = null;
          peer = null;
          global.gc();
        }
        const reports = await Promise.all(pending);
        reports.forEach(report => assert.ok(report instanceof Map));
        if (${collect}) {
          endpoints = null;
          peer = null;
          await new Promise(resolve => setImmediate(resolve));
          global.gc();
          await new Promise(resolve => setImmediate(resolve));
          global.gc();
        }
      })().catch(error => { console.error(error); process.exitCode = 1; });
    `;
    var result = spawnSync(process.execPath, ['--expose-gc', '-e', script], {
      cwd: path.resolve(__dirname, '..'),
      encoding: 'utf8',
      timeout: 10000
    });
    t.equal(result.status, 0, result.stderr || String(result.error || 'process exited cleanly'));
  }
  t.end();
});

test('worker exit leaves the main thread WebRTC usable', function (t) {
  const script = `
    const assert = require('node:assert/strict')
    const { Worker } = require('node:worker_threads')
    const { RTCPeerConnection } = require('./')

    const workerSource = function () {
      const assert = require('node:assert/strict')
      const { workerData } = require('node:worker_threads')
      const { RTCPeerConnection } = require('./')

      if (workerData) {
        const peer = new RTCPeerConnection({ iceServers: [] })
        try {
          const { receiver } = peer.addTransceiver('audio', { direction: 'recvonly' })
          assert.equal(receiver.track.kind, 'audio')
          peer.createDataChannel('worker').close()
        } finally {
          peer.close()
        }
      }
    }

    function runWorker (usePeer) {
      return new Promise((resolve, reject) => {
        const worker = new Worker('(' + workerSource.toString() + ')()', {
          eval: true,
          workerData: usePeer
        })
        worker.once('error', reject)
        worker.once('exit', code => {
          if (code === 0) resolve()
          else reject(new Error('Worker exited with code ' + code))
        })
      })
    }

    async function main () {
      await runWorker(false)
      const peer = new RTCPeerConnection({ iceServers: [] })
      try {
        await runWorker(true)
        const { receiver } = peer.addTransceiver('audio', { direction: 'recvonly' })
        assert.equal(receiver.track.kind, 'audio')
        peer.createDataChannel('main').close()
        assert.equal((await peer.createOffer()).type, 'offer')
      } finally {
        peer.close()
      }
      assert.throws(() => peer.createDataChannel('closed'), { name: 'InvalidStateError' })
      console.log('peer closed')
    }

    main().catch(err => {
      console.error(err)
      process.exitCode = 1
    })
  `
  const result = spawnSync(process.execPath, ['-e', script], {
    cwd: path.resolve(__dirname, '..'),
    encoding: 'utf8',
    timeout: 15000
  })
  t.ifError(result.error)
  t.equal(result.signal, null, result.stderr || 'no worker teardown signal')
  t.equal(result.status, 0, result.stdout + result.stderr)
  t.equal(result.stdout.trim(), 'peer closed')
  t.end()
})
