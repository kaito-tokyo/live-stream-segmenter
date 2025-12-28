/**
 * Payload builder for YouTube Data API v3: liveBroadcasts.insert.
 * Uses dayjs for date handling.
 * * Default values are pre-filled based on the API documentation and common usage patterns.
 */
export class LiveBroadcastBuilder {
  /**
   * Constructor (sets required fields and populates optional fields with defaults).
   * * Default settings applied:
   * - Privacy: public
   * - Made For Kids: false
   * - Monitor Stream: enabled (requires "testing" stage before live)
   * - DVR: enabled
   * - Embed: enabled
   * - Record From Start: enabled
   * - Latency: normal
   * - Projection: rectangular
   * - Auto Start/Stop: false
   * @param {string} title - The broadcast title (snippet.title).
   * @param {string|Date|dayjs.Dayjs} scheduledStartTime - The scheduled start time (snippet.scheduledStartTime).
   * @param {string} [privacyStatus='private'] - Privacy status ('public', 'private', 'unlisted').
   */
  constructor(title, scheduledStartTime, privacyStatus = 'private') {
    if (!title)
      throw new Error('Title is required for LiveBroadcastBuilder');

    if (!scheduledStartTime)
      throw new Error(
        'Scheduled start time is required for LiveBroadcastBuilder'
      );

    this.payload = {
      snippet: {
        title: title,
        scheduledStartTime: this._formatDate(scheduledStartTime),
        // description is optional, defaults to undefined (API treats as empty)
      },
      status: {
        privacyStatus: privacyStatus,
        selfDeclaredMadeForKids: false, // Defaulting to false (General audience)
      },
      contentDetails: {
        // API Default: true. Enables the preview player in YouTube Studio.
        monitorStream: {
          enableMonitorStream: true,
          broadcastStreamDelayMs: 0,
        },
        // API Default: true. Allows embedding on 3rd party sites.
        enableEmbed: true,
        // API Default: true. Allows viewers to seek back during live.
        enableDvr: true,
        // API Default: true. Automatically archives the stream as a video.
        recordFromStart: true,
        // Default: rectangular (standard video).
        projection: 'rectangular',
        // Default: normal (standard latency).
        latencyPreference: 'normal',
        // Default: false. Safer to manually start/stop.
        enableAutoStart: false,
        enableAutoStop: false,
        // Default: disabled.
        closedCaptionsType: 'closedCaptionsDisabled',
      },
    };
  }

  /* --- Snippet (Basic Details) --- */

  /**
   * Sets the broadcast description.
   * @param {string} description - The description text.
   * @returns {this}
   */
  setDescription(description) {
    this.payload.snippet.description = description;
    return this;
  }

  /**
   * Sets the scheduled end time.
   * @param {string|Date|dayjs.Dayjs} scheduledEndTime - The scheduled end time.
   * @returns {this}
   */
  setScheduledEndTime(scheduledEndTime) {
    this.payload.snippet.scheduledEndTime = this._formatDate(scheduledEndTime);
    return this;
  }

  /* --- Status (Status Settings) --- */

  /**
   * Overrides the self-declared "made for kids" status.
   * Defaults to false in constructor.
   * @param {boolean} isMadeForKids - Whether the content is made for kids.
   * @returns {this}
   */
  setSelfDeclaredMadeForKids(isMadeForKids) {
    this.payload.status.selfDeclaredMadeForKids = isMadeForKids;
    return this;
  }

  /* --- Content Details (Stream Configuration) --- */

  /**
   * Configures the monitor stream (preview).
   * Defaults to enabled=true, delay=0 in constructor.
   * @param {boolean} enable - Whether to enable the monitor stream.
   * @param {number} [delayMs=0] - Broadcast delay in milliseconds.
   * @returns {this}
   */
  setMonitorStream(enable, delayMs = 0) {
    this.payload.contentDetails.monitorStream = {
      enableMonitorStream: enable,
      broadcastStreamDelayMs: delayMs,
    };
    return this;
  }

  /**
   * Sets whether the broadcast should start automatically when the bound stream starts.
   * Defaults to false in constructor.
   * @param {boolean} enable - Enable auto-start.
   * @returns {this}
   */
  setEnableAutoStart(enable) {
    this.payload.contentDetails.enableAutoStart = enable;
    return this;
  }

  /**
   * Sets whether the broadcast should stop automatically when the bound stream stops.
   * Defaults to false in constructor.
   * @param {boolean} enable - Enable auto-stop.
   * @returns {this}
   */
  setEnableAutoStop(enable) {
    this.payload.contentDetails.enableAutoStop = enable;
    return this;
  }

  /**
   * Sets whether DVR (seek/rewind) is enabled for viewers.
   * Defaults to true in constructor.
   * @param {boolean} enable - Enable DVR.
   * @returns {this}
   */
  setEnableDvr(enable) {
    this.payload.contentDetails.enableDvr = enable;
    return this;
  }

  /**
   * Sets whether the broadcast can be embedded on other sites.
   * Defaults to true in constructor.
   * @param {boolean} enable - Enable embedding.
   * @returns {this}
   */
  setEnableEmbed(enable) {
    this.payload.contentDetails.enableEmbed = enable;
    return this;
  }

  /**
   * Sets whether to automatically record (archive) the broadcast from the start.
   * Defaults to true in constructor.
   * @param {boolean} enable - Enable recording from start.
   * @returns {this}
   */
  setRecordFromStart(enable) {
    this.payload.contentDetails.recordFromStart = enable;
    return this;
  }

  /**
   * Sets closed captions availability.
   * Defaults to 'closedCaptionsDisabled' in constructor.
   * @param {boolean} enable - Enable closed captions.
   * @returns {this}
   */
  setEnableClosedCaptions(enable) {
    this.payload.contentDetails.closedCaptionsType = enable
      ? 'closedCaptionsHttpPost'
      : 'closedCaptionsDisabled';
    return this;
  }

  /**
   * Sets the latency preference.
   * Defaults to 'normal' in constructor.
   * @param {'normal'|'low'|'ultraLow'} preference - Latency mode.
   * @returns {this}
   */
  setLatencyPreference(preference) {
    this.payload.contentDetails.latencyPreference = preference;
    return this;
  }

  /**
   * Sets the projection format.
   * Defaults to 'rectangular' in constructor.
   * @param {'rectangular'|'360'} projection - Projection type.
   * @returns {this}
   */
  setProjection(projection) {
    this.payload.contentDetails.projection = projection;
    return this;
  }

  /* --- Helper Methods --- */

  /**
   * Builds and returns the final JSON payload.
   * @returns {Object} The request body object.
   */
  build() {
    return this.payload;
  }

  /**
   * Safely formats the input date into an ISO 8601 string using dayjs.
   * @param {string|Date|dayjs.Dayjs} input - The date input.
   * @returns {string|undefined} The ISO 8601 formatted date string.
   * @private
   */
  _formatDate(input) {
    if (!input) return undefined;

    if (typeof input === 'object' && typeof input.toISOString === 'function') {
      if (typeof input.isValid === 'function' && !input.isValid()) {
        throw new Error('Invalid date object provided');
      }
      return input.toISOString();
    }

    if (input instanceof Date) {
      if (isNaN(input.getTime())) {
        throw new Error('Invalid Date object provided');
      }
      return input.toISOString();
    }

    const d = new Date(input);
    if (isNaN(d.getTime())) {
      throw new Error(`Invalid date format: ${input}`);
    }
    return d.toISOString();
  }
}
