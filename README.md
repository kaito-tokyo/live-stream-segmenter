# Live Stream Segmenter

[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPL%203.0%20or%20later-blue.svg)](LICENSE.GPL-3.0-or-later)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.MIT)

An OBS Studio plugin that automatically segments YouTube live streams to maintain broadcast archives within YouTube's 12-hour stream limit.

## Overview

**Live Stream Segmenter** enables OBS streamers to automatically renew their YouTube broadcasts at regular intervals. This is essential for long-duration streams, as YouTube limits individual stream archives to 12 hours. By segmenting broadcasts before reaching this limit, streamers can preserve their entire streaming history without manual intervention.

### Key Features

- **Automatic Broadcast Renewal**: Automatically creates new YouTube broadcasts at configured intervals
- **Seamless Transitions**: Transitions from current broadcast to the next with minimal interruption
- **Configurable Intervals**: Set custom segmentation intervals to suit your streaming schedule
- **YouTube API Integration**: Direct integration with YouTube Live Streaming API
- **OAuth2 Authentication**: Secure authentication using Google OAuth2
- **Custom Scripting**: JavaScript-based event handlers for customized broadcast creation and management
- **Real-time Status Monitoring**: Track current and next broadcast status in the OBS dock
- **Manual Segmentation**: Option to manually trigger a segment at any time

## Why Use This Plugin?

YouTube automatically limits individual live stream recordings to 12 hours. For streamers who broadcast for extended periods (marathons, charity streams, 24-hour events), this means:

- **Without this plugin**: Your stream archive gets cut off at 12 hours
- **With this plugin**: Your stream automatically transitions to new broadcasts, preserving all content

This plugin ensures that:
- All your streaming content is archived on YouTube
- Viewers experience smooth transitions between segments
- Your VODs are preserved for future viewing
- You maintain compliance with YouTube's technical limitations

## Requirements

- **OBS Studio** 31.1.1 or later
- **YouTube Account** with API access enabled
- **Operating System**:
  - Windows 10/11 (x64)
  - macOS 11 (Big Sur) or later
  - Ubuntu 24.04 or later

## Installation

### From Release

1. Download the latest release for your platform from the [Releases page](https://github.com/kaito-tokyo/live-stream-segmenter/releases)
2. Extract the archive
3. Copy the plugin files to your OBS plugins directory:
   - **Windows**: `%APPDATA%\obs-studio\plugins\`
   - **macOS**: `~/Library/Application Support/obs-studio/plugins/`
   - **Linux**: `~/.config/obs-studio/plugins/`
4. Restart OBS Studio

### From Source

See [Building from Source](#building-from-source) section below.

## Configuration

### 1. YouTube API Setup

Before using the plugin, you must enable YouTube API access:

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select an existing one
3. Enable the **YouTube Data API v3**
4. Create OAuth 2.0 credentials (Desktop application type)
5. Download the credentials JSON file

### 2. Plugin Configuration

1. Open OBS Studio
2. Navigate to **View** → **Docks** → **Live Stream Segmenter** to open the plugin dock
3. Click **Settings** button
4. Configure the following:
   - **OAuth Credentials**: Drop or paste your Google OAuth2 client credentials JSON
   - **Segmentation Interval**: Set how often to renew broadcasts (default: 11 hours)
   - **Event Handlers**: Configure JavaScript event handlers for broadcast creation

### 3. Authentication

1. Click **Start** in the Live Stream Segmenter dock
2. A browser window will open for Google OAuth2 authentication
3. Sign in with your YouTube account
4. Grant the requested permissions
5. Return to OBS Studio

## Usage

### Starting Automatic Segmentation

1. Open the **Live Stream Segmenter** dock in OBS
2. Configure your stream settings as usual in OBS
3. Click **Start** in the Live Stream Segmenter dock
4. The plugin will:
   - Create the first broadcast on YouTube
   - Set up OBS to stream to it
   - Schedule the next broadcast renewal
5. Start streaming in OBS normally

### During Streaming

- **Status Monitor**: View current broadcast status and countdown to next segment
- **Broadcast Schedule**: See details for current and next broadcasts
- **Operation Log**: Monitor plugin activities and any issues
- **Manual Segment**: Click **Segment Now** to immediately transition to the next broadcast

### Stopping Segmentation

1. Click **Stop** in the Live Stream Segmenter dock
2. The plugin will:
   - Complete the current broadcast
   - Stop the automatic renewal process
3. Stop streaming in OBS

## Scripting

The plugin supports ES6 module-based JavaScript event handlers for advanced customization. Scripts use the QuickJS-ng runtime with built-in modules for YouTube API, localStorage, and date handling.

### Built-in Modules

- `builtin:youtube` - LiveBroadcastBuilder for creating broadcast configurations
- `builtin:localStorage` - Persistent key-value storage across segments
- `builtin:dayjs` - Date/time manipulation library

### Event Handlers

#### `onInitYouTubeStreamSegmenter()`

Called once when the segmenter initializes. Configure global settings:

```javascript
export function onInitYouTubeStreamSegmenter() {
  return {
    segmentIntervalMilliseconds: 6 * 60 * 60 * 1000, // 6 hours
  };
}
```

#### `onCreateYouTubeLiveBroadcastInitial()`

Called to create the first broadcast when starting segmentation:

```javascript
import { LiveBroadcastBuilder } from "builtin:youtube";

export function onCreateYouTubeLiveBroadcastInitial() {
  const title = "My Live Stream - Part 1";
  const description = "24-hour live streaming!";
  
  return {
    YouTubeLiveBroadcast: new LiveBroadcastBuilder(title, new Date(), 'private')
      .setDescription(description)
      .setEnableDvr(true)
      .setEnableEmbed(true)
      .build()
  };
}
```

#### `onSetYouTubeThumbnailInitial({ LiveBroadcast })`

Called to set a thumbnail for the initial broadcast:

```javascript
export function onSetYouTubeThumbnailInitial({ LiveBroadcast: { id: videoId } }) {
  return {
    videoId,
    thumbnailFile: "/path/to/thumbnail.jpg"
  };
}
```

#### `onCreateYouTubeLiveBroadcastInitialNext()`

Called to create the second (next) broadcast during initialization:

```javascript
export function onCreateYouTubeLiveBroadcastInitialNext() {
  const title = "My Live Stream - Part 2";
  
  return {
    YouTubeLiveBroadcast: new LiveBroadcastBuilder(title, new Date(), 'private')
      .setDescription("Continuing the stream...")
      .build()
  };
}
```

#### `onSetYouTubeThumbnailInitialNext({ LiveBroadcast })`

Called to set a thumbnail for the second broadcast.

#### `onCreateYouTubeLiveBroadcastNext()`

Called each time a new segment is created during streaming:

```javascript
export function onCreateYouTubeLiveBroadcastNext() {
  // Track segment count using localStorage
  const index = (Number(localStorage.getItem("index")) || 0) + 1;
  localStorage.setItem("index", index);
  
  const title = `My Stream - Part ${index}`;
  
  return {
    YouTubeLiveBroadcast: new LiveBroadcastBuilder(title, new Date(), 'public')
      .setDescription("Live streaming with automatic segmentation")
      .setEnableDvr(false)  // Disable DVR for faster processing
      .setEnableEmbed(true)
      .build()
  };
}
```

#### `onSetYouTubeThumbnailNext({ LiveBroadcast })`

Called to set a thumbnail for each new segment.

### Complete Example

```javascript
import { LiveBroadcastBuilder } from "builtin:youtube";

function createYouTubeLiveBroadcast() {
  // Track segment count using localStorage
  const index = (Number(localStorage.getItem("index")) || 0) + 1;
  localStorage.setItem("index", index);
  
  const title = `24-Hour Stream ${index}`;
  const description = `Live streaming 24/7!\n\nTwitter: https://twitter.com/yourusername\nDiscord: https://discord.gg/yourserver`;
  
  return new LiveBroadcastBuilder(title, new Date(), 'private')
    .setDescription(description)
    .setEnableDvr(false)
    .setEnableEmbed(false)
    .build();
}

export function onInitYouTubeStreamSegmenter() {
  return {
    segmentIntervalMilliseconds: 11 * 60 * 60 * 1000, // 11 hours
  };
}

export function onCreateYouTubeLiveBroadcastInitial() {
  return { YouTubeLiveBroadcast: createYouTubeLiveBroadcast() };
}

export function onSetYouTubeThumbnailInitial({ LiveBroadcast: { id: videoId } }) {
  return {
    videoId,
    thumbnailFile: "/path/to/thumbnail.jpg"
  };
}

export function onCreateYouTubeLiveBroadcastInitialNext() {
  return { YouTubeLiveBroadcast: createYouTubeLiveBroadcast() };
}

export function onSetYouTubeThumbnailInitialNext({ LiveBroadcast: { id: videoId } }) {
  return {
    videoId,
    thumbnailFile: "/path/to/thumbnail.jpg"
  };
}

export function onCreateYouTubeLiveBroadcastNext() {
  return { YouTubeLiveBroadcast: createYouTubeLiveBroadcast() };
}

export function onSetYouTubeThumbnailNext({ LiveBroadcast: { id: videoId } }) {
  return {
    videoId,
    thumbnailFile: "/path/to/thumbnail.jpg"
  };
}
```

### LiveBroadcastBuilder API

The `LiveBroadcastBuilder` class provides a fluent API for configuring broadcasts:

- `new LiveBroadcastBuilder(title, scheduledStartTime, privacyStatus)` - Constructor
- `.setDescription(description)` - Set broadcast description
- `.setScheduledEndTime(endTime)` - Set scheduled end time
- `.setSelfDeclaredMadeForKids(boolean)` - Set "made for kids" flag
- `.setMonitorStream(enable, delayMs)` - Configure preview stream
- `.setEnableAutoStart(boolean)` - Enable/disable auto-start
- `.setEnableAutoStop(boolean)` - Enable/disable auto-stop
- `.setEnableDvr(boolean)` - Enable/disable DVR (seek during live)
- `.setEnableEmbed(boolean)` - Enable/disable embedding on other sites
- `.setRecordFromStart(boolean)` - Enable/disable automatic archiving
- `.setEnableClosedCaptions(boolean)` - Enable/disable closed captions
- `.setLatencyPreference('normal'|'low'|'ultraLow')` - Set stream latency
- `.setProjection('rectangular'|'360')` - Set video projection type
- `.build()` - Return the final broadcast configuration

## Building from Source

### Prerequisites

- **CMake** 3.28 or later
- **C++20** compatible compiler
- **C17** compatible compiler
- **clang-format-19** (for code formatting)
- **gersemi** (for CMake formatting)
- Platform-specific tools:
  - **Windows**: Visual Studio 2022
  - **macOS**: Xcode 16.0
  - **Linux**: GCC/Clang, ninja-build, pkg-config

### Build Instructions

#### macOS

```bash
cmake --preset macos
cmake --build --preset macos
ctest --preset macos --rerun-failed --output-on-failure
```

To install for testing with OBS:

```bash
cmake --build --preset macos && \
cmake --install build_macos --config RelWithDebInfo \
  --prefix "$HOME/Library/Application Support/obs-studio/plugins"
```

#### Windows

```bash
cmake --preset windows
cmake --build --preset windows
ctest --preset windows
```

#### Linux

```bash
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

### Development

For development builds with Address Sanitizer (macOS):

```bash
cmake --preset macos-dev
cmake --build --preset macos-dev
```

## Architecture

The plugin consists of several key components:

- **Controller**: Main plugin logic and YouTube stream segmentation loop
- **UI**: Qt-based user interface (dock, settings dialog, OAuth flow)
- **YouTube API**: Client library for YouTube Live Streaming API v3
- **Google Auth**: OAuth2 authentication and token management
- **Scripting**: JavaScript runtime for event handlers
- **Store**: Data persistence for authentication and broadcast state

## Troubleshooting

### Authentication Issues

- Ensure your Google OAuth2 credentials are valid
- Check that YouTube Data API v3 is enabled in your Google Cloud project
- Verify the OAuth consent screen is properly configured

### Streaming Not Starting

- Check the Operation Log in the plugin dock for error messages
- Ensure you're authenticated (see status in System Monitor)
- Verify your YouTube account can create live broadcasts

### Segmentation Not Happening

- Check the configured segmentation interval
- Monitor the countdown timer in the Broadcast Schedule section
- Look for errors in the Operation Log

### Manual Recovery

If the plugin encounters issues:
1. Click **Stop** to halt automatic segmentation
2. Manually complete any active broadcasts in YouTube Studio
3. Reconfigure authentication if needed
4. Click **Start** to restart

## License

This project uses a dual-license model:

- **GPL-3.0-or-later**: Main plugin code, OBS integration, and UI components
- **MIT License**: Reusable libraries (Logger, CurlHelper, YouTube API, Google Auth)

See [LICENSE](LICENSE), [LICENSE.GPL-3.0-or-later](LICENSE.GPL-3.0-or-later), and [LICENSE.MIT](LICENSE.MIT) for details.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Follow the development guidelines in `.github/copilot-instructions.md`
4. Format code with `clang-format-19` and `gersemi`
5. Ensure tests pass
6. Submit a pull request

## Support

- **Documentation**: [https://kaito-tokyo.github.io/live-stream-segmenter/](https://kaito-tokyo.github.io/live-stream-segmenter/)
- **Issues**: [GitHub Issues](https://github.com/kaito-tokyo/live-stream-segmenter/issues)
- **Email**: umireon@kaito.tokyo

## Author

**Kaito Udagawa** (umireon@kaito.tokyo)

## Acknowledgments

Built with:
- [OBS Studio](https://obsproject.com/)
- [Qt Framework](https://www.qt.io/)
- [cURL](https://curl.se/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [QuickJS-ng](https://github.com/quickjs-ng/quickjs)
