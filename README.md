# NS-ctest-mock

Atmosphère sysmodule that answers Nintendo Switch **connection test** (`ctest`) on `127.0.0.1:80`.

Use it together with DNS MITM so the console can mark Wi‑Fi as connected **without** talking to Nintendo’s ctest servers, and **without** enabling sys-patch / pack `nifm ctest` patches (those patches are a known cause of Core #3 100% / broken SaltyNX FPS overlays).

Based on [qwertycxz/NS-ctest-mock](https://github.com/qwertycxz/NS-ctest-mock) (Apache-2.0), with fixes for modern Atmosphère / HOS (tested on **AMS 1.12.1** + **FW 22.1.0**).

---

## English

### What it does

1. You redirect ctest hostnames to loopback via `atmosphere/hosts`.
2. This sysmodule listens on `127.0.0.1:80` and returns:

```http
HTTP/1.0 200 OK
Content-Length: 2
X-Organization: Nintendo

ok
```

3. Nintendo’s connection test passes locally. Other Nintendo hosts can stay blocked.

### Why this fork / rebuild

Upstream release often **does not start or does not listen** on current firmware:

| Issue | Effect |
| --- | --- |
| `bsdSocketExempt` | Creates an already-shut-down socket → `bind`/`listen` never works |
| Busy `continue` on socket errors | Can spin Core #3 |
| `diagAbort` during early `boot2` if `bsd` is not ready yet | Process dies silently |
| NPDM `optimize_memory_allocation` / odd title id | Loader may refuse to launch the module |

This tree uses a normal `bsdSocket`, sleeps on failures, initializes BSD lazily in `main`, keeps the thread on **CPU 3**, and installs under title id `00FF000063746573`.

### Install (prebuilt)

1. Download `NS-ctest-mock.tar` from Releases (or use the copy in the repo root / `dist/`).
2. Extract to the **root of the SD card** so you get:

```text
atmosphere/contents/00FF000063746573/exefs.nsp
atmosphere/contents/00FF000063746573/toolbox.json
atmosphere/contents/00FF000063746573/flags/boot2.flag
```

3. Add ctest redirects to your active hosts file, for example `atmosphere/hosts/emummc.txt` and/or `atmosphere/hosts/emummc_<id>.txt` (Atmosphère picks the emuMMC-specific file when present). See [`examples/hosts-ctest.txt`](examples/hosts-ctest.txt).
4. Keep **sys-patch nifm / ctest patches disabled**.
5. If Windows wrote the SD over UMS/MTP/FTP, run **Hekate → Tools → Archive Bit → Fix** if modules misbehave.
6. Reboot into emuMMC. In Tesla → Sysmodules, **NS-ctest-mock** should be ON / running.
7. Reconnect Wi‑Fi and verify connection test succeeds while Nintendo ctest stays local.

### Build from source

Needs [devkitPro](https://devkitpro.org/) (`DEVKITPRO` + libnx) or Docker image `devkitpro/devkita64`:

```bash
make clean
make APP_ID=00FF000063746573
```

Artifacts land in `atmosphere/contents/00FF000063746573/`.

### Important notes

- This only mocks **HTTP ctest**. It is not a full Nintendo online stack.
- If Status Monitor / SaltyNX still shows Core #3 at 100% and FPS `n/d` **with Wi‑Fi on**, that is often a separate issue: **sigpatches/sys-patch + non-primary console + linked account + network**. Fixing that may require unlinking the account (e.g. with Linkalho) or registering the console as primary — this sysmodule does not solve that by itself.
- Blank PRODINFO / DNS blocks are your own risk profile; keep Nintendo services blocked if that is your setup.

### Credits

- Original project: [qwertycxz/NS-ctest-mock](https://github.com/qwertycxz/NS-ctest-mock)
- License: Apache License 2.0 (see `LICENSE`)

---

## Русский

### Зачем это нужно

Sysmodule отвечает на **тест соединения** Nintendo Switch на `127.0.0.1:80`. Вместе с DNS MITM консоль считает Wi‑Fi рабочим **без** обращения к серверам ctest Nintendo и **без** включения патчей `nifm ctest` в sys-patch/паках (они часто дают Core #3 100% и ломают FPS в Status Monitor / SaltyNX).

Основано на [qwertycxz/NS-ctest-mock](https://github.com/qwertycxz/NS-ctest-mock) (Apache-2.0). Эта сборка поправлена под актуальные Atmosphère / HOS (проверено на **AMS 1.12.1** + **FW 22.1.0**).

### Что делает

1. В `atmosphere/hosts` ctest-хосты смотрят на loopback.
2. Модуль слушает `127.0.0.1:80` и отдаёт ответ `ok` с заголовком `X-Organization: Nintendo`.
3. Тест соединения проходит локально. Остальные хосты Nintendo можно по-прежнему резать в hosts.

### Чем отличается от стока

| Проблема в upstream | Что бывает |
| --- | --- |
| `bsdSocketExempt` | Сокет уже shutdown → listen не встаёт |
| `continue` без sleep при ошибках | Может крутить Core #3 |
| `diagAbort`, если `bsd` ещё не готов на boot2 | Модуль тихо умирает |
| Тяжёлые флаги NPDM / ASCII title id | Loader может не запустить процесс |

Здесь обычный `bsdSocket`, sleep на ошибках, ленивый init BSD в `main`, поток на **CPU 3**, title id `00FF000063746573`.

### Установка (готовый бинарь)

1. Возьми `NS-ctest-mock.tar` из Releases (или из корня репо / `dist/`).
2. Распакуй в **корень SD**.
3. Добавь редиректы ctest в активный hosts (`emummc.txt` и/или `emummc_<id>.txt`). Пример: [`examples/hosts-ctest.txt`](examples/hosts-ctest.txt).
4. **nifm / ctest в sys-patch — выключены**.
5. При копировании с Windows при необходимости: Hekate → Archive Bit → Fix.
6. Ребут в emuMMC → Sysmodules → **NS-ctest-mock** должен быть запущен.
7. Подключи Wi‑Fi и проверь, что тест связи проходит.

### Сборка

Нужен devkitPro или Docker `devkitpro/devkita64`:

```bash
make clean
make APP_ID=00FF000063746573
```

### Важно

- Мокается только **HTTP ctest**, не весь онлайн Nintendo.
- Если при Wi‑Fi Core #3 ~100% и FPS `n/d`, это часто **отдельная** история: sys-patch + не Primary + linked-аккаунт + сеть. Лечится unlink (например Linkalho) или Primary — этот sysmodule сам по себе FPS не чинит.
- Blank PRODINFO и блокировки Nintendo — на твоей ответственности.

### Благодарности

- Оригинал: [qwertycxz/NS-ctest-mock](https://github.com/qwertycxz/NS-ctest-mock)
- Лицензия: Apache License 2.0 (`LICENSE`)
