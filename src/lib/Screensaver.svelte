<script>
  import { onMount, onDestroy } from 'svelte';
  import Settings from './Settings.svelte';
  
  let canvas;
  let ctx;
  let wasmModule;
  let animationId;
  let initPipes, updatePipes, getFramebuffer, cleanupPipes;
  let setFadeSpeed, setSpawnRate, setTurnProbability, setMaxPipes, setAnimationSpeed;
  let showSettings = true;
  let animationDelay = 1000 / 60; // Default 60 FPS
  
  onMount(async () => {
    // Load WASM module
    try {
      console.log('Loading WASM module...');
      
      // Import ES6 module
      const createPipesModule = (await import('../wasm/pipes.js')).default;
      wasmModule = await createPipesModule();
      console.log('WASM module loaded:', wasmModule);
      
      // Get exported functions
      initPipes = wasmModule.cwrap('init_pipes', null, ['number', 'number']);
      updatePipes = wasmModule.cwrap('update_pipes', null, []);
      getFramebuffer = wasmModule.cwrap('get_framebuffer', 'number', []);
      cleanupPipes = wasmModule.cwrap('cleanup_pipes', null, []);
      
      // Get parameter setters
      setFadeSpeed = wasmModule.cwrap('set_fade_speed', null, ['number']);
      setSpawnRate = wasmModule.cwrap('set_spawn_rate', null, ['number']);
      setTurnProbability = wasmModule.cwrap('set_turn_probability', null, ['number']);
      setMaxPipes = wasmModule.cwrap('set_max_pipes', null, ['number']);
      setAnimationSpeed = wasmModule.cwrap('set_animation_speed', null, ['number']);
      
      // Set up canvas
      canvas = document.getElementById('pipes-canvas');
      ctx = canvas.getContext('2d');
      console.log('Canvas context:', ctx);
      
      // Set canvas size
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
      console.log('Canvas size:', canvas.width, 'x', canvas.height);
      
      // Initialize pipes system
      initPipes(canvas.width, canvas.height);
      console.log('Pipes initialized');
      
      // Start animation
      animate();
      
    } catch (error) {
      console.error('Failed to load WASM module:', error);
    }
  });
  
  onDestroy(() => {
    if (animationId) {
      cancelAnimationFrame(animationId);
    }
    if (cleanupPipes) {
      cleanupPipes();
    }
  });
  
  function animate() {
    if (!wasmModule || !ctx) return;
    
    // Update pipes
    updatePipes();
    
    // Get framebuffer pointer
    const bufferPtr = getFramebuffer();
    
    if (bufferPtr && wasmModule.HEAPU8) {
      // Create ImageData from WASM memory using HEAPU8
      const dataLength = canvas.width * canvas.height * 4;
      const buffer = new Uint8ClampedArray(wasmModule.HEAPU8.buffer, bufferPtr, dataLength);
      
      const imageData = new ImageData(buffer, canvas.width, canvas.height);
      ctx.putImageData(imageData, 0, 0);
    }
    
    // Use setTimeout for custom FPS control
    setTimeout(() => {
      animationId = requestAnimationFrame(animate);
    }, animationDelay);
  }
  
  function updateAnimationSpeed(fps) {
    animationDelay = 1000 / fps;
    if (setAnimationSpeed) {
      setAnimationSpeed(fps);
    }
  }
  
  function handleResize() {
    if (canvas && initPipes) {
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
      initPipes(canvas.width, canvas.height);
    }
  }
</script>

<svelte:window on:resize={handleResize} />

{#if showSettings && setFadeSpeed}
  <Settings 
    {setFadeSpeed}
    {setSpawnRate}
    {setTurnProbability}
    {setMaxPipes}
    setAnimationSpeed={updateAnimationSpeed}
  />
{/if}

<button 
  class="settings-toggle" 
  on:click={() => showSettings = !showSettings}
>
  {showSettings ? 'Hide' : 'Show'} Settings
</button>

<style>
  .settings-toggle {
    position: absolute;
    top: 20px;
    right: 20px;
    background: rgba(0, 0, 0, 0.8);
    color: white;
    border: 1px solid rgba(255, 255, 255, 0.2);
    padding: 0.5rem 1rem;
    border-radius: 4px;
    cursor: pointer;
    z-index: 100;
  }
  
  .settings-toggle:hover {
    background: rgba(0, 0, 0, 0.9);
    border-color: rgba(255, 255, 255, 0.4);
  }
</style>