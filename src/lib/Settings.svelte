<script>
  export let setFadeSpeed;
  export let setSpawnRate;
  export let setTurnProbability;
  export let setMaxPipes;
  export let setAnimationSpeed;
  
  let fadeSpeed = 1;
  let spawnRate = 10;
  let turnProbability = 30;
  let maxPipes = 3;
  let animationSpeed = 60;
  
  function updateFadeSpeed() {
    setFadeSpeed(fadeSpeed);
  }
  
  function updateSpawnRate() {
    setSpawnRate(spawnRate);
  }
  
  function updateTurnProbability() {
    setTurnProbability(turnProbability);
  }
  
  function updateMaxPipes() {
    setMaxPipes(maxPipes);
  }
  
  function updateAnimationSpeed() {
    setAnimationSpeed(animationSpeed);
  }
</script>

<div class="settings-panel">
  <h3>Settings</h3>
  
  <div class="setting">
    <label for="fade-speed">Fade Speed</label>
    <input 
      id="fade-speed"
      type="range" 
      min="0" 
      max="10" 
      bind:value={fadeSpeed} 
      on:input={updateFadeSpeed}
    />
    <span class="value">{fadeSpeed}</span>
  </div>
  
  <div class="setting">
    <label for="spawn-rate">Spawn Rate</label>
    <input 
      id="spawn-rate"
      type="range" 
      min="1" 
      max="50" 
      bind:value={spawnRate} 
      on:input={updateSpawnRate}
    />
    <span class="value">{spawnRate}%</span>
  </div>
  
  <div class="setting">
    <label for="turn-prob">Turn Probability</label>
    <input 
      id="turn-prob"
      type="range" 
      min="10" 
      max="90" 
      bind:value={turnProbability} 
      on:input={updateTurnProbability}
    />
    <span class="value">{turnProbability}%</span>
  </div>
  
  <div class="setting">
    <label for="max-pipes">Max Pipes</label>
    <input 
      id="max-pipes"
      type="range" 
      min="1" 
      max="10" 
      bind:value={maxPipes} 
      on:input={updateMaxPipes}
    />
    <span class="value">{maxPipes}</span>
  </div>
  
  <div class="setting">
    <label for="anim-speed">Animation Speed (FPS)</label>
    <input 
      id="anim-speed"
      type="range" 
      min="10" 
      max="120" 
      step="10"
      bind:value={animationSpeed} 
      on:input={updateAnimationSpeed}
    />
    <span class="value">{animationSpeed}</span>
  </div>
</div>

<style>
  .settings-panel {
    position: absolute;
    top: 20px;
    right: 20px;
    background: rgba(0, 0, 0, 0.8);
    color: white;
    padding: 20px;
    border-radius: 8px;
    min-width: 300px;
    border: 1px solid rgba(255, 255, 255, 0.2);
  }
  
  h3 {
    margin: 0 0 15px 0;
    font-size: 1.2rem;
  }
  
  .setting {
    margin-bottom: 15px;
    display: grid;
    grid-template-columns: 120px 1fr 50px;
    align-items: center;
    gap: 10px;
  }
  
  label {
    font-size: 0.9rem;
  }
  
  input[type="range"] {
    width: 100%;
    height: 4px;
    background: rgba(255, 255, 255, 0.2);
    outline: none;
    -webkit-appearance: none;
    appearance: none;
    cursor: pointer;
  }
  
  input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 16px;
    height: 16px;
    background: #ff3e00;
    cursor: pointer;
    border-radius: 50%;
  }
  
  input[type="range"]::-moz-range-thumb {
    width: 16px;
    height: 16px;
    background: #ff3e00;
    cursor: pointer;
    border-radius: 50%;
    border: none;
  }
  
  .value {
    text-align: right;
    font-size: 0.9rem;
    color: #ff3e00;
  }
</style>