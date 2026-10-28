<p align="center">
  <img src="https://raw.githubusercontent.com/skitzellez/xbox-game-vault/main/GameVault/art/logo-banner.png" alt="Game Vault" width="720">
</p>

<h3 align="center">Game Vault · Flash</h3>

<p align="center">
  <b>Flash games on your Xbox, with the controller.</b><br>
  The Flash player for <a href="https://github.com/skitzellez/xbox-game-vault">Game Vault</a> on <b>Xbox Series X|S</b> in Dev Mode:
  pick a Flash game in Game Vault and it plays here, then you are back in Game Vault.
</p>

<p align="center">
  <a href="https://github.com/skitzellez/Flash-GameVault/releases/latest"><img alt="Download Game Vault · Flash" src="https://img.shields.io/badge/DOWNLOAD-Flash-107C10?style=for-the-badge&logo=xbox&logoColor=white"></a>
  &nbsp;
  <a href="#-installation"><img alt="How to install" src="https://img.shields.io/badge/HOW%20TO-INSTALL-0B7DDA?style=for-the-badge"></a>
  &nbsp;
  <a href="https://www.patreon.com/Skitzelles"><img alt="Support on Patreon" src="https://img.shields.io/badge/SUPPORT-Patreon-F96854?style=for-the-badge&logo=patreon&logoColor=white"></a>
</p>

<p align="center">
  <a href="https://github.com/skitzellez/Flash-GameVault/releases"><img alt="Latest release" src="https://img.shields.io/github/v/release/skitzellez/Flash-GameVault?label=latest&color=107C10"></a>
  <a href="https://github.com/skitzellez/xbox-game-vault#-related-projects"><img alt="Downloads of Game Vault and its emulators" src="https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fskitzellez%2Fgame-vault-updates%2Fmain%2Fbadges%2Fdownloads-all.json"></a>
  <a href="https://discord.gg/RmhhWgguvw"><img alt="Discord" src="https://img.shields.io/badge/Discord-join-5865F2?logo=discord&logoColor=white"></a>
  <a href="LICENSE"><img alt="License: PolyForm Noncommercial" src="https://img.shields.io/badge/license-PolyForm%20Noncommercial-blue"></a>
  <img alt="Status: new" src="https://img.shields.io/badge/status-new-orange">
</p>

<p align="center">
  <img src="art/screens/flash-tab.jpg" alt="The FLASH shelf in Game Vault on the TV" width="900">
</p>

<table align="center">
  <tr>
    <td align="center" width="33%"><h3>🔌 1. Add</h3>Put your <code>.swf</code> games on a USB drive or your PC / NAS.</td>
    <td align="center" width="33%"><h3>🎯 2. Pick</h3>They show up on Game Vault's <b>FLASH</b> shelf.</td>
    <td align="center" width="33%"><h3>▶️ 3. Play</h3>Press A. Flash starts with the game, ready for the controller.</td>
  </tr>
</table>

## ✨ Features

- Plays Flash games (`.swf`) with [Ruffle](https://ruffle.rs), the Flash Player emulator, in the console's own web engine.
- Games from a USB drive or straight from your PC or NAS (Game Vault copies them over, they are small).
- Made for the controller: no keyboard or mouse needed.
- Hold **View + Menu** to go back to Game Vault.

## 🎮 Controls

| Controller | Plays as |
|---|---|
| D-pad / left stick | Arrow keys |
| A | Space |
| B / X / Y | X / Z / C |
| Menu | Enter |
| LB / RB | Shift / Ctrl |
| Right stick | Mouse pointer |
| RT (or LT) | Mouse click |
| Hold View + Menu | Back to Game Vault |

## 📦 Installation

- **Easiest:** in Game Vault press **RT**, pick **Flash** and add it with your phone. Game Vault offers updates there too.
- **By hand:** in Device Portal, **My games & apps → Add**, choose `FlashForGameVault_<version>_x64.appx` from the
  [latest release](https://github.com/skitzellez/Flash-GameVault/releases/latest), then **Start** it once.

Requirements: Xbox Series X|S in Dev Mode with [Game Vault](https://github.com/skitzellez/xbox-game-vault) 0.16.1.297 or newer.

## 🔒 Privacy

Flash games run inside the app, and links a game tries to open are blocked. Game Vault · Flash itself sends nothing anywhere.

## 🔗 Related projects

- [Game Vault](https://github.com/skitzellez/xbox-game-vault): the game library this player belongs to.
- [Game Vault Discord](https://discord.gg/RmhhWgguvw): help, news and reports.

## 🙏 Credits

- [Ruffle](https://ruffle.rs) (MIT / Apache-2.0), the Flash Player emulator: `web/ruffle`, licences in `web/ruffle/LICENSE_MIT` and `web/ruffle/LICENSE_APACHE`.
- [Microsoft WebView2](https://learn.microsoft.com/microsoft-edge/webview2/) (BSD licence, `Microsoft.Web.WebView2.Core.dll`).

No games are included. Play Flash games you are allowed to have.

## 📄 Licence

Game Vault · Flash's own code: [PolyForm Noncommercial 1.0.0](LICENSE), Copyright (c) 2026 Skitzelles: free to use and
share, not for sale, and the notice stays. Ruffle and WebView2 keep their own licences.
