/*
 * Live Stream Segmenter - Scripting Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// ----------------------------------------------------------------------
// Type Definitions
// ----------------------------------------------------------------------

/**
 * Type definition for the database object injected from C++.
 * @typedef {Object} ScriptingDatabase
 * @property {(sql: string, ...args: (string|number|null|ArrayBuffer)[]) => any[]} query
 * Executes an SQL query and returns the result as an array of rows. Used for SELECT statements.
 * @property {(sql: string, ...args: (string|number|null|ArrayBuffer)[]) => { changes: number, lastInsertId: number }} execute
 * Executes an SQL statement and returns modification info. Used for INSERT/UPDATE/DELETE statements.
 */

// ----------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------

(function () {
  /** * Reference to the global db object.
   * @type {ScriptingDatabase}
   */
  // @ts-ignore (db is injected globally from C++, so ignore TS checks)
  const db = globalThis.db;

  // Do nothing if the db object is undefined (e.g. server-side rendering context).
  if (typeof db === "undefined") return;

  // Prefix to avoid conflicts with user-defined tables or SQLite system tables.
  const TABLE_NAME = "__sys_local_storage";

  // Initialize the table.
  // Keys and values are stored as strings to comply with the Web Storage API specification.
  db.execute(
    `CREATE TABLE IF NOT EXISTS ${TABLE_NAME} (key TEXT PRIMARY KEY, value TEXT)`,
  );

  // Internal flag to control instantiation.
  let allowInstantiation = false;

  /**
   * Implementation of the Web Storage API's Storage interface.
   * Uses SQLite as a backend to persist data.
   * @implements {Storage}
   */
  class Storage {
    constructor() {
      if (!allowInstantiation) {
        throw new TypeError(
          "Illegal constructor: Storage cannot be instantiated directly.",
        );
      }
    }

    /**
     * Returns the number of data items stored in the Storage object.
     * @returns {number} The number of stored items.
     */
    get length() {
      const res = db.query(`SELECT COUNT(*) as count FROM ${TABLE_NAME}`);
      return res && res.length > 0 ? Number(res[0].count) : 0;
    }

    /**
     * Returns the name of the nth key in the storage.
     * @param {number} n - The index of the key to retrieve (0-based).
     * @returns {string|null} The name of the key, or null if the index is out of range.
     */
    key(n) {
      // Use OFFSET to retrieve the nth key.
      const res = db.query(
        `SELECT key FROM ${TABLE_NAME} LIMIT 1 OFFSET ?`,
        Number(n),
      );
      return res && res.length > 0 ? String(res[0].key) : null;
    }

    /**
     * Returns the current value associated with the given key.
     * @param {string} k - The key of the value to retrieve.
     * @returns {string|null} The value associated with the key, or null if the key does not exist.
     */
    getItem(k) {
      const res = db.query(
        `SELECT value FROM ${TABLE_NAME} WHERE key = ?`,
        String(k),
      );
      return res && res.length > 0 ? String(res[0].value) : null;
    }

    /**
     * Adds a key and value to storage, or updates the value if the key already exists.
     * Both key and value are automatically converted to strings.
     * @param {string} k - The name of the key.
     * @param {string} v - The value to store.
     * @returns {void}
     */
    setItem(k, v) {
      // Use INSERT OR REPLACE to achieve upsert behavior.
      db.execute(
        `INSERT OR REPLACE INTO ${TABLE_NAME} (key, value) VALUES (?, ?)`,
        String(k),
        String(v),
      );
    }

    /**
     * Removes the specified key from storage.
     * @param {string} k - The name of the key to remove.
     * @returns {void}
     */
    removeItem(k) {
      db.execute(`DELETE FROM ${TABLE_NAME} WHERE key = ?`, String(k));
    }

    /**
     * Removes all keys from storage.
     * @returns {void}
     */
    clear() {
      db.execute(`DELETE FROM ${TABLE_NAME}`);
    }
  }

  // Expose the class definition globally.
  // @ts-ignore
  globalThis.Storage = Storage;

  // Create the localStorage singleton, allowing instantiation only within this scope.
  allowInstantiation = true;
  // @ts-ignore
  globalThis.localStorage = new Storage();
  allowInstantiation = false;
})();
