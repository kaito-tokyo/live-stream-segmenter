#include <coroutine>
#include <queue>
#include <mutex>
#include <functional>
#include <optional>

// Qtへの依存はここ（とcppファイル）だけに絞る
#include <QCoreApplication>
#include <QThreadPool>
#include <QRunnable>
#include <QDebug> // デバッグ表示用

// ==========================================
// 1. Qt依存部: スケジューリング・ブリッジ
// ==========================================
namespace QtScheduler {
// A. スレッドプールでコルーチンを再開させるタスク
class ResumeTask : public QRunnable {
	std::coroutine_handle<> m_handle;

public:
	ResumeTask(std::coroutine_handle<> h) : m_handle(h) { setAutoDelete(true); }
	void run() override
	{
		if (m_handle && !m_handle.done())
			m_handle.resume();
	}
};
} // namespace QtScheduler

// ==========================================
// 2. 共有コンテキスト (STLのみで構成)
// ==========================================
struct WorkerContext {
	// データ構造は標準C++
	std::queue<int> messages;
	std::mutex mtx;
	std::coroutine_handle<> waitingHandle = nullptr;

	// 外部(Qt Main)からの投入口
	void push(int msg)
	{
		std::unique_lock lock(mtx);
		messages.push(msg);

		if (waitingHandle) {
			auto h = waitingHandle;
			waitingHandle = nullptr;
			lock.unlock(); // ロック解除してからスケジュール

			// ★ここだけスイッチ機能としてQt依存部を呼ぶ
			QtScheduler::schedule_on_worker(h);
		}
	}
};

// ==========================================
// 3. Awaitables (STLコンテキスト操作 + スイッチ)
// ==========================================

// メッセージ受信待ち
struct next_message {
	WorkerContext *ctx;

	bool await_ready() { return false; }

	void await_suspend(std::coroutine_handle<> h)
	{
		std::unique_lock lock(ctx->mtx);
		if (!ctx->messages.empty()) {
			// メッセージがあれば即時再開 (現在のスレッドで継続)
			lock.unlock();
			h.resume();
		} else {
			// なければハンドルを預けてサスペンド
			ctx->waitingHandle = h;
		}
	}

	int await_resume()
	{
		std::lock_guard lock(ctx->mtx);
		// 規約通り、ここに来る時は必ずデータがある前提
		int val = ctx->messages.front();
		ctx->messages.pop();
		return val;
	}
};

// メインスレッドへスイッチ
struct switch_to_main {
	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h) { QtScheduler::schedule_on_main(h); }
	void await_resume() {}
};

// ワーカースレッドへスイッチ
struct switch_to_worker {
	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h) { QtScheduler::schedule_on_worker(h); }
	void await_resume() {}
};

// ==========================================
// 4. コルーチン定義 (標準仕様)
// ==========================================
struct StateMachine {
	struct promise_type {
		StateMachine get_return_object() { return StateMachine{}; }
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_never final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() { std::terminate(); }
	};
};
