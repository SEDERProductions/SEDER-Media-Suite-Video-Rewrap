import { createHash } from 'node:crypto';
import {
  chmodSync,
  copyFileSync,
  existsSync,
  mkdirSync,
  readFileSync,
  readdirSync,
  rmSync,
  statSync,
  writeFileSync,
} from 'node:fs';
import { basename, dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const releaseDir = join(root, 'release');
const packageJson = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8'));
const ref = process.env.GITHUB_REF_NAME || '';
const version = (/^v\d/.test(ref) ? ref : packageJson.version).replace(/^v/, '');
const app = {
  packageName: 'seder-video-rewrap',
  appName: 'SEDER Media Suite Video Rewrap',
  bundleId: 'com.sederproductions.videorewrap',
};

function platformInfo() {
  const platformMap = { darwin: 'macos', win32: 'windows', linux: 'linux' };
  const archMap = { arm64: 'arm64', x64: 'x64' };
  return {
    platform: platformMap[process.platform] || process.platform,
    architecture: archMap[process.arch] || process.arch,
    executableExt: process.platform === 'win32' ? '.exe' : '',
  };
}

function findTool(name) {
  const qtPrefix = process.env.QT_PREFIX;
  if (qtPrefix) {
    const candidate = join(qtPrefix, 'bin', process.platform === 'win32' ? `${name}.exe` : name);
    if (existsSync(candidate)) return candidate;
  }
  try {
    return execFileSync(process.platform === 'win32' ? 'where' : 'which', [name], { encoding: 'utf8' })
      .split(/\r?\n/)
      .find(Boolean);
  } catch {
    return null;
  }
}

function ensureTool(name, message) {
  const tool = findTool(name);
  if (!tool) throw new Error(message || `Required tool not found: ${name}`);
  return tool;
}

function sha256(path) {
  return createHash('sha256').update(readFileSync(path)).digest('hex');
}

function archiveDirectory(inputDir, outputPath) {
  rmSync(outputPath, { force: true });
  if (process.platform === 'win32') {
    execFileSync('powershell', [
      '-NoProfile',
      '-Command',
      `Compress-Archive -Path "${inputDir}\\*" -DestinationPath "${outputPath}" -Force`,
    ], { stdio: 'inherit' });
    return;
  }
  ensureTool('zip', 'zip is required to create release archives.');
  execFileSync('zip', ['-r', '-9', outputPath, basename(inputDir)], {
    cwd: dirname(inputDir),
    stdio: 'inherit',
  });
}

function configureAndBuild() {
  const cmake = ensureTool(
    'cmake',
    'CMake and Qt 6 are required. On macOS: brew install qt cmake ninja && export QT_PREFIX="$(brew --prefix qt)"',
  );
  const buildDir = join(root, 'qt', 'build', 'release');
  const args = [
    '-S', 'qt',
    '-B', buildDir,
    '-DCMAKE_BUILD_TYPE=Release',
  ];
  if (process.env.QT_PREFIX) args.push(`-DCMAKE_PREFIX_PATH=${process.env.QT_PREFIX}`);
  execFileSync(cmake, args, { cwd: root, stdio: 'inherit' });
  execFileSync(cmake, ['--build', buildDir, '--config', 'Release'], { cwd: root, stdio: 'inherit' });
}

function builtExecutable(info) {
  const base = join(root, 'qt', 'build', 'release', 'bin');
  // Visual Studio multi-config generators place Release binaries in a Release/
  // subdirectory on Windows, while Ninja/Make place them directly in bin/.
  const releaseSub = join(base, 'Release');
  const candidates = [
    join(releaseSub, `${app.packageName}${info.executableExt}`),
    join(base, `${app.packageName}${info.executableExt}`),
  ];
  for (const candidate of candidates) {
    if (existsSync(candidate)) return candidate;
  }
  return candidates[0]; // fallback to original path for a clear error message
}

function createMacBundle(executable, outputRoot) {
  const bundle = join(outputRoot, `${app.appName}.app`);
  const contents = join(bundle, 'Contents');
  const macos = join(contents, 'MacOS');
  const resources = join(contents, 'Resources');
  rmSync(bundle, { recursive: true, force: true });
  mkdirSync(macos, { recursive: true });
  mkdirSync(resources, { recursive: true });
  copyFileSync(executable, join(macos, app.packageName));
  chmodSync(join(macos, app.packageName), 0o755);
  if (existsSync(join(root, 'assets', 'icon.svg'))) {
    copyFileSync(join(root, 'assets', 'icon.svg'), join(resources, 'icon.svg'));
  }
  const hasIcns = existsSync(join(root, 'assets', 'icon.icns'));
  if (hasIcns) {
    copyFileSync(join(root, 'assets', 'icon.icns'), join(resources, 'icon.icns'));
  }
  const iconKey = hasIcns ? '\n  <key>CFBundleIconFile</key>\n  <string>icon.icns</string>' : '';
  writeFileSync(join(contents, 'PkgInfo'), 'APPL????');
  writeFileSync(join(contents, 'Info.plist'), `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>en</string>
  <key>CFBundleDisplayName</key>
  <string>${app.appName}</string>
  <key>CFBundleExecutable</key>
  <string>${app.packageName}</string>
  <key>CFBundleIdentifier</key>
  <string>${app.bundleId}</string>${iconKey}
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>${app.appName}</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>${version}</string>
  <key>CFBundleVersion</key>
  <string>${version}</string>
  <key>LSMinimumSystemVersion</key>
  <string>11.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
`);
  const macdeployqt = ensureTool('macdeployqt', 'macdeployqt is required to package macOS Qt releases.');
  execFileSync(macdeployqt, [bundle], { stdio: 'inherit' });
  execFileSync('codesign', [
    '--force', '--deep', '--options', 'runtime',
    '--identifier', app.bundleId,
    '--sign', '-',
    '--timestamp=none',
    bundle,
  ], { stdio: 'inherit' });
  execFileSync('codesign', ['--verify', '--deep', '--strict', '--verbose=2', bundle], {
    stdio: 'inherit',
  });
  return bundle;
}

function findVcRedistDlls() {
  const dllNames = ['msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll'];
  const found = [];
  // Search Visual Studio redist directories (GitHub Actions / dev machines)
  const vsPaths = [
    'C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Redist\\MSVC',
    'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Redist\\MSVC',
    'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Redist\\MSVC',
    'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Redist\\MSVC',
    'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Community\\VC\\Redist\\MSVC',
    'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Professional\\VC\\Redist\\MSVC',
  ];
  for (const vsPath of vsPaths) {
    if (!existsSync(vsPath)) continue;
    const versions = readdirSync(vsPath);
    for (const versionDir of versions.sort().reverse()) {
      const archDir = join(vsPath, versionDir, 'x64');
      if (!existsSync(archDir)) continue;
      let allFound = true;
      for (const dll of dllNames) {
        const p = join(archDir, dll);
        if (!existsSync(p)) { allFound = false; break; }
      }
      if (allFound) {
        for (const dll of dllNames) found.push(join(archDir, dll));
        return found;
      }
    }
  }
  // Fallback to System32 (already-installed runtimes)
  const sys32 = 'C:\\Windows\\System32';
  if (existsSync(sys32)) {
    let allFound = true;
    for (const dll of dllNames) {
      const p = join(sys32, dll);
      if (!existsSync(p)) { allFound = false; break; }
    }
    if (allFound) {
      for (const dll of dllNames) found.push(join(sys32, dll));
      return found;
    }
  }
  return [];
}

function packageWindows(executable, outputRoot) {
  const stage = join(outputRoot, `${app.packageName}-v${version}-windows-${platformInfo().architecture}`);
  rmSync(stage, { recursive: true, force: true });
  mkdirSync(stage, { recursive: true });
  console.log(`[package] Copying executable to ${stage}`);
  copyFileSync(executable, join(stage, `${app.packageName}.exe`));
  const windeployqt = ensureTool('windeployqt', 'windeployqt is required to package Windows Qt releases.');
  console.log(`[package] Running windeployqt: ${windeployqt}`);
  execFileSync(windeployqt, ['--release', '--qmldir', join(root, 'qt', 'qml'), join(stage, `${app.packageName}.exe`)], {
    stdio: 'inherit',
  });
  const vcDlls = findVcRedistDlls();
  if (vcDlls.length > 0) {
    console.log(`[package] Copying VC++ runtime DLLs to ${stage}`);
    for (const dll of vcDlls) {
      copyFileSync(dll, join(stage, basename(dll)));
    }
  } else {
    console.warn('[package] VC++ runtime DLLs not found. The app may crash on machines without Visual C++ Redistributables installed.');
  }
  try {
    signWindowsExecutables(stage);
  } catch (err) {
    console.warn(`[package] Code signing failed (non-fatal): ${err.message}`);
  }
  return stage;
}

function signWindowsExecutables(dir) {
  // Read the staging directory from the SEDER_SIGN_DIR env var rather than
  // string-interpolating it into the PS source. PowerShell single-quoted
  // strings only escape apostrophes by doubling them; an apostrophe in the
  // checkout path (legal on Windows) would close the literal early. Using an
  // env var also avoids needing to escape backslashes.
  const ps = `
    $ErrorActionPreference = 'Stop'
    $stageDir = $env:SEDER_SIGN_DIR
    if (-not $stageDir) { throw 'SEDER_SIGN_DIR not set' }
    # Guard against environments where the Cert: drive is unavailable
    if (-not (Get-PSDrive Cert -ErrorAction SilentlyContinue)) { throw 'Cert: drive is not available' }
    $cert = New-SelfSignedCertificate \`
      -Subject 'CN=SEDER Productions, O=SEDER Productions, C=GB' \`
      -Type CodeSigningCert -KeyUsage DigitalSignature \`
      -KeyAlgorithm RSA -KeyLength 2048 \`
      -CertStoreLocation Cert:\\CurrentUser\\My \`
      -NotAfter (Get-Date).AddYears(5)
    $signtool = (Get-ChildItem "\${env:ProgramFiles(x86)}\\Windows Kits\\10\\bin" -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue |
      Where-Object { $_.FullName -match 'x64\\\\signtool.exe$' } | Select-Object -First 1).FullName
    if (-not $signtool) { $signtool = 'signtool.exe' }
    Get-ChildItem -LiteralPath $stageDir -Recurse -Filter *.exe | ForEach-Object {
      & $signtool sign /fd SHA256 /sha1 $cert.Thumbprint /tr http://timestamp.digicert.com /td SHA256 $_.FullName
      if ($LASTEXITCODE -ne 0) { throw "signtool failed for $($_.FullName)" }
    }
  `;
  execFileSync('powershell', ['-NoProfile', '-Command', ps], {
    stdio: 'inherit',
    env: { ...process.env, SEDER_SIGN_DIR: dir },
  });
}

function packageLinux(executable, outputRoot) {
  const info = platformInfo();
  const appDir = join(outputRoot, `${app.packageName}.AppDir`);
  rmSync(appDir, { recursive: true, force: true });
  mkdirSync(join(appDir, 'usr', 'bin'), { recursive: true });
  mkdirSync(join(appDir, 'usr', 'share', 'applications'), { recursive: true });
  mkdirSync(join(appDir, 'usr', 'share', 'icons', 'hicolor', 'scalable', 'apps'), { recursive: true });
  copyFileSync(executable, join(appDir, 'usr', 'bin', app.packageName));
  chmodSync(join(appDir, 'usr', 'bin', app.packageName), 0o755);
  if (existsSync(join(root, 'assets', 'icon.svg'))) {
    copyFileSync(join(root, 'assets', 'icon.svg'), join(appDir, 'usr', 'share', 'icons', 'hicolor', 'scalable', 'apps', `${app.packageName}.svg`));
  }
  writeFileSync(join(appDir, 'usr', 'share', 'applications', `${app.packageName}.desktop`), `[Desktop Entry]
Type=Application
Name=${app.appName}
Exec=${app.packageName}
Icon=${app.packageName}
Categories=AudioVideo;Video;
`);

  const linuxdeploy = findTool('linuxdeploy-x86_64.AppImage') || findTool('linuxdeploy');
  if (linuxdeploy) {
    console.log(`[package] Found linuxdeploy: ${linuxdeploy}`);
    chmodSync(linuxdeploy, 0o755);
    const args = [
      '--appimage-extract-and-run',
      '--appdir', appDir,
      '--plugin', 'qt',
      '--output', 'appimage',
    ];
    console.log(`[package] Running linuxdeploy with args: ${args.join(' ')}`);
    try {
      execFileSync(linuxdeploy, args, {
        cwd: outputRoot,
        env: {
          ...process.env,
          OUTPUT: join(outputRoot, `${app.packageName}-v${version}-${info.platform}-${info.architecture}.AppImage`),
        },
        stdio: 'inherit',
      });
    } catch (err) {
      console.warn(`[package] linuxdeploy failed, falling back to plain archive: ${err.message}`);
    }
    const appImage = join(outputRoot, `${app.packageName}-v${version}-${info.platform}-${info.architecture}.AppImage`);
    if (existsSync(appImage)) return appImage;
    console.log('[package] AppImage not created, falling back to plain archive');
  } else {
    console.log('[package] linuxdeploy not found, creating plain archive');
  }

  return appDir;
}

function packageRelease() {
  const info = platformInfo();
  console.log(`[package] Platform: ${info.platform}, Arch: ${info.architecture}`);
  const executable = builtExecutable(info);
  console.log(`[package] Executable path: ${executable}`);
  if (!existsSync(executable)) throw new Error(`Missing built executable: ${executable}`);
  rmSync(releaseDir, { recursive: true, force: true });
  mkdirSync(releaseDir, { recursive: true });
  const workDir = join(releaseDir, 'work');
  mkdirSync(workDir, { recursive: true });

  let distributable;
  let finalPath;
  if (info.platform === 'macos') {
    distributable = createMacBundle(executable, workDir);
    finalPath = join(releaseDir, `${app.packageName}-v${version}-${info.platform}-${info.architecture}.zip`);
    archiveDirectory(distributable, finalPath);
  } else if (info.platform === 'windows') {
    distributable = packageWindows(executable, workDir);
    finalPath = join(releaseDir, `${app.packageName}-v${version}-${info.platform}-${info.architecture}.zip`);
    archiveDirectory(distributable, finalPath);
  } else if (info.platform === 'linux') {
    distributable = packageLinux(executable, workDir);
    if (distributable.endsWith('.AppImage')) {
      finalPath = join(releaseDir, basename(distributable));
      copyFileSync(distributable, finalPath);
    } else {
      finalPath = join(releaseDir, `${app.packageName}-v${version}-${info.platform}-${info.architecture}.zip`);
      archiveDirectory(distributable, finalPath);
    }
  } else {
    throw new Error(`Unsupported release platform: ${info.platform}`);
  }

  const checksum = sha256(finalPath);
  writeFileSync(`${finalPath}.sha256`, `${checksum}  ${basename(finalPath)}\n`);
  rmSync(workDir, { recursive: true, force: true });
  console.log(`Created ${finalPath}`);
  console.log(`SHA-256 ${checksum}`);
}

console.log('[package] Starting configure and build...');
configureAndBuild();
console.log('[package] Starting package release...');
packageRelease();
