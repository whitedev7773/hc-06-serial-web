<script lang="ts">
	// Web Serial API related state
	let port: SerialPort | null = null;
	let reader: ReadableStreamDefaultReader | null = null;
	let writer: WritableStreamDefaultWriter | null = null;
	let keepReading = false;

	// Common state
	let isConnected = false;
	let deviceName = '';
	let keyInput = '';
	let valueInput = '';
	let mappings = [
		{ key: 'w', value: '앞으로' },
		{ key: 's', value: '뒤로' },
		{ key: 'a', value: '왼쪽으로' },
		{ key: 'd', value: '오른쪽으로' }
	];
	let serialLog = '';
	let pressedKey = '';
	let lastSent: { key: string; value: string } | null = null;

	async function onConnectClick() {
		if (isConnected) {
			await disconnect();
		} else {
			await connect();
		}
	}

	async function connect() {
		try {
			if (!('serial' in navigator)) {
				alert('이 브라우저에서는 Web Serial API를 사용할 수 없습니다!');
				return;
			}
			logToSerial('연결할 COM 포트를 선택하세요...');
			port = await navigator.serial.requestPort();
			await port.open({ baudRate: 9600 });
			logToSerial('포트가 성공적으로 열렸습니다.');

			isConnected = true;
			deviceName = `COM Port`; // Generic name for serial port

			if (!port.writable || !port.readable) {
				logToSerial('포트에 쓸 수 없거나 읽을 수 없습니다.');
				return;
			}
			keepReading = true;
			writer = port.writable.getWriter();
			readLoop(); // Start the reading loop
		} catch (error) {
			if (error instanceof Error) {
				logToSerial(`오류: ${error.message}`);
			}
			await disconnect();
		}
	}

	async function disconnect() {
		keepReading = false;

		if (reader) {
			try {
				await reader.cancel();
			} catch (error) {
				/* Ignore error */
			}
			// The lock is automatically released on cancel.
			reader = null;
		}

		if (writer) {
			try {
				await writer.releaseLock();
			} catch (error) {
				/* Ignore error */
			}
			writer = null;
		}

		if (port) {
			try {
				await port.close();
			} catch (error) {
				/* Ignore error */
			}
			port = null;
			logToSerial('포트가 닫혔습니다.');
		}

		resetState();
	}

	async function readLoop() {
		if (!port?.readable) return;
		reader = port.readable.getReader();

		try {
			while (port.readable && keepReading) {
				const { value, done } = await reader.read();
				if (done) {
					break;
				}
				const decoder = new TextDecoder('utf-8');
				const receivedText = decoder.decode(value);
				logToSerial(`수신: ${receivedText}`);
			}
		} catch (error) {
			if (error instanceof Error && error.name !== 'AbortError') {
				logToSerial(`읽기 오류: ${error.message}`);
			}
		} finally {
			if (reader) reader.releaseLock();
		}
	}

	function resetState() {
		isConnected = false;
		deviceName = '';
	}

	function logToSerial(message: string) {
		const timestamp = new Date().toLocaleTimeString();
		serialLog += `[${timestamp}] ${message}\n`;
		// Auto-scroll textarea
		const textarea = document.querySelector('textarea');
		if (textarea) textarea.scrollTop = textarea.scrollHeight;
	}

	async function sendData(data: string) {
		if (!writer) {
			logToSerial('데이터를 보낼 수 없습니다: 포트가 열려있지 않음.');
			return;
		}
		try {
			const encoder = new TextEncoder();
			await writer.write(encoder.encode(data));
			logToSerial(`전송: ${data}`);
		} catch (error) {
			if (error instanceof Error) {
				logToSerial(`전송 오류: ${error.message}`);
			}
		}
	}

	function addMapping() {
		if (keyInput && valueInput && !mappings.find((m) => m.key === keyInput)) {
			mappings = [...mappings, { key: keyInput, value: valueInput }];
			keyInput = '';
			valueInput = '';
		}
	}

	function removeMapping(index: number) {
		mappings = mappings.filter((_, i) => i !== index);
	}

	async function handleKeydown(event: KeyboardEvent) {
		if (event.target instanceof HTMLInputElement || event.target instanceof HTMLTextAreaElement) {
			return;
		}

		const mapping = mappings.find((m) => m.key === event.key);
		if (mapping && isConnected) {
			pressedKey = event.key;
			lastSent = mapping;
			await sendData(mapping.value);
			setTimeout(() => {
				pressedKey = '';
			}, 300);
		}
	}
</script>

<svelte:window on:keydown={handleKeydown} on:beforeunload={disconnect} />

<svelte:head>
	<title>HC-06 웹 시리얼</title>
</svelte:head>

<div class="container">
	<header>
		<h1>HC-06 웹 시리얼 인터페이스 (Web Serial API)</h1>
		<p>웹 기반으로 HC-06 블루투스 모듈과 통신하는 인터페이스입니다.</p>
	</header>

	<main>
		<div class="top-section">
			<!-- Connection Section -->
			<section class="card">
				<h2>연결</h2>
				<div class="connection-status">
					상태:
					<span class={isConnected ? 'connected' : 'disconnected'}>
						{isConnected ? `${deviceName}에 연결됨` : '연결 끊김'}
					</span>
				</div>
				<button class="button" on:click={onConnectClick}>
					{isConnected ? '연결 끊기' : 'COM 포트에 연결'}
				</button>
			</section>

			<!-- Serial Monitor Section -->
			<section class="card">
				<h2>시리얼 모니터</h2>
				<p>HC-06 모듈에서 수신된 데이터를 표시합니다.</p>
				<textarea readonly bind:value={serialLog} placeholder="데이터 수신 대기 중..."></textarea>
				<button class="button" on:click={() => (serialLog = '')}>로그 지우기</button>
			</section>
		</div>

		<!-- Input Status Section -->
		<section class="card">
			<h2>입력 상태</h2>
			{#if !isConnected}
				<p class="status-idle">연결 후 키를 누르면 입력 상태가 표시됩니다.</p>
			{:else if lastSent}
				<p class="status-active">
					<strong>{lastSent.key}</strong> 키를 눌러 <strong>"{lastSent.value}"</strong> 값을 전송했습니다.
				</p>
			{:else}
				<p class="status-idle">매핑된 키를 눌러주세요...</p>
			{/if}
		</section>

		<!-- Key Mapping Section -->
		<section class="card">
			<h2>키 매핑</h2>
			<p>키보드 키를 눌렀을 때 전송할 문자열 값을 지정합니다.</p>
			<div class="mapping-input">
				<input type="text" placeholder="키 (예: 'w')" bind:value={keyInput} maxlength="1" />
				<input
					type="text"
					placeholder="값 (예: '앞으로')"
					bind:value={valueInput}
					on:keydown={(e) => e.key === 'Enter' && addMapping()}
				/>
				<button class="button-small" on:click={addMapping}>추가</button>
			</div>
			<ul class="mapping-list">
				{#each mappings as mapping, i}
					<li class:active={pressedKey === mapping.key}>
						<span><strong>{mapping.key}</strong> 키 → <strong>"{mapping.value}"</strong> 전송</span>
						<button class="button-small-danger" on:click={() => removeMapping(i)}> 삭제 </button>
					</li>
				{/each}
			</ul>
		</section>

		<!-- Instructions Section -->
		<section class="card">
			<h2>사용 설명서</h2>
			<ol>
				<li>
					<strong>HC-06 페어링:</strong> HC-06 모듈의 전원을 켜고, 컴퓨터의 **Windows 블루투스 설정**에서
					기기와 먼저 페어링하세요.
				</li>
				<li>
					<strong>COM 포트 확인:</strong> Windows '장치 관리자'를 열고 '포트 (COM & LPT)' 섹션에서 HC-06에
					할당된 COM 포트 번호(예: COM3, COM4)를 확인하세요. 'Standard Serial over Bluetooth link' 라고
					표시될 수 있습니다.
				</li>
				<li>
					<strong>포트 연결:</strong> 'COM 포트에 연결' 버튼을 클릭하세요. 브라우저 팝업이 나타나면 위에서
					확인한 COM 포트를 선택하고 연결하세요.
				</li>
				<li>
					<strong>키 매핑 및 테스트:</strong> 키를 매핑하고 눌러서 데이터를 전송하거나, HC-06에서 오는
					데이터를 모니터링하세요.
				</li>
			</ol>
			<p class="note">
				<strong>참고:</strong> 이 앱은 Web Serial API를 사용하며, 호환되는 브라우저(Chrome, Edge, Opera
				등)와 보안 컨텍스트(HTTPS 또는 localhost)가 필요합니다.
			</p>
		</section>
	</main>
</div>

<style>
	:root {
		--primary-color: #007bff;
		--highlight-color: #d6eaff;
		--danger-color: #dc3545;
		--success-color: #28a745;
		--background-color: #f4f7f9;
		--text-color: #333;
		--card-bg: #ffffff;
		--border-radius: 8px;
		--box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
	}

	.container {
		max-width: 1200px;
		margin: 2rem auto;
		padding: 0 1rem;
		font-family:
			-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans',
			'Helvetica Neue', sans-serif;
		color: var(--text-color);
		background-color: var(--background-color);
	}

	header {
		text-align: center;
		margin-bottom: 2rem;
	}

	.card {
		background: var(--card-bg);
		border-radius: var(--border-radius);
		box-shadow: var(--box-shadow);
		padding: 1.5rem;
		margin-bottom: 1.5rem;
	}

	.top-section {
		display: flex;
		flex-wrap: wrap;
		gap: 1.5rem;
		margin-bottom: 1.5rem;
	}

	.top-section > .card {
		flex: 1;
		min-width: 350px;
		margin-bottom: 0;
	}

	.connection-status {
		margin-bottom: 1rem;
	}

	.connected {
		color: var(--success-color);
		font-weight: bold;
	}

	.disconnected {
		color: var(--danger-color);
		font-weight: bold;
	}

	.status-idle {
		color: #6c757d;
	}

	.status-active {
		color: var(--primary-color);
		font-weight: bold;
	}

	.button {
		background-color: var(--primary-color);
		color: white;
		border: none;
		padding: 10px 15px;
		border-radius: 5px;
		cursor: pointer;
		font-size: 1rem;
		transition: background-color 0.2s;
	}
	.button:hover {
		background-color: #0056b3;
	}

	.mapping-input {
		display: flex;
		gap: 0.5rem;
		margin-bottom: 1rem;
	}

	.mapping-input input {
		flex-grow: 1;
		padding: 8px;
		border: 1px solid #ccc;
		border-radius: 5px;
	}

	.button-small {
		background-color: #6c757d;
		color: white;
		border: none;
		padding: 8px 12px;
		border-radius: 5px;
		cursor: pointer;
		transition: background-color 0.2s;
	}
	.button-small:hover {
		background-color: #5a6268;
	}

	.button-small-danger {
		background-color: var(--danger-color);
		color: white;
		border: none;
		padding: 5px 10px;
		border-radius: 5px;
		cursor: pointer;
		font-size: 0.8rem;
	}

	.mapping-list {
		list-style: none;
		padding: 0;
	}

	.mapping-list li {
		display: flex;
		justify-content: space-between;
		align-items: center;
		padding: 0.5rem;
		border-bottom: 1px solid #eee;
		transition: background-color 0.2s;
	}

	.mapping-list li.active {
		background-color: var(--highlight-color);
	}

	textarea {
		width: 100%;
		height: 150px;
		border-radius: 5px;
		border: 1px solid #ccc;
		padding: 8px;
		margin-bottom: 1rem;
		font-family: 'Courier New', Courier, monospace;
		scroll-behavior: smooth;
	}

	.note {
		background-color: #e7f3fe;
		border-left: 4px solid var(--primary-color);
		padding: 1rem;
		font-size: 0.9rem;
	}

	ol {
		padding-left: 20px;
	}

	ol li {
		margin-bottom: 0.5rem;
	}

	@media (max-width: 768px) {
		.top-section {
			flex-direction: column;
		}
	}
</style>
