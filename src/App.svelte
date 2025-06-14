<script>
  import Screensaver from './lib/Screensaver.svelte';
  
  let currentView = 'welcome';

  function goToScreensaver() {
    currentView = 'screensaver';
  }

  function goToWelcome() {
    currentView = 'welcome';
  }
</script>

{#if currentView === 'welcome'}
  <main class="welcome">
    <h1>Windows Pipes Screensaver</h1>
    <p>Experience the nostalgia of the classic Windows 3D Pipes screensaver</p>
    <button class="go-button" on:click={goToScreensaver}>
      Go to Screensaver
    </button>
  </main>
{:else}
  <div class="screensaver-container">
    <button class="back-button" on:click={goToWelcome}>Back</button>
    <canvas id="pipes-canvas"></canvas>
    <Screensaver />
  </div>
{/if}

<style>
  .welcome {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    height: 100vh;
    text-align: center;
    padding: 2rem;
  }

  h1 {
    font-size: 3rem;
    margin-bottom: 1rem;
    color: #ff3e00;
  }

  p {
    font-size: 1.2rem;
    color: #666;
    margin-bottom: 2rem;
  }

  .go-button {
    background-color: #ff3e00;
    color: white;
    border: none;
    padding: 1rem 2rem;
    font-size: 1.1rem;
    border-radius: 8px;
    cursor: pointer;
    transition: background-color 0.3s ease;
  }

  .go-button:hover {
    background-color: #e63300;
  }

  .screensaver-container {
    position: relative;
    width: 100vw;
    height: 100vh;
    background-color: #000;
  }

  .back-button {
    position: absolute;
    top: 20px;
    left: 20px;
    z-index: 10;
    background-color: rgba(255, 255, 255, 0.2);
    color: white;
    border: 1px solid rgba(255, 255, 255, 0.3);
    padding: 0.5rem 1rem;
    border-radius: 4px;
    cursor: pointer;
    transition: background-color 0.3s ease;
  }

  .back-button:hover {
    background-color: rgba(255, 255, 255, 0.3);
  }

  #pipes-canvas {
    width: 100%;
    height: 100%;
    display: block;
  }
</style>