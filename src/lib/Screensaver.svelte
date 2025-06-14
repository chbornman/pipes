<script>
  import { onMount, onDestroy } from 'svelte';
  import Settings from './Settings.svelte';
  
  let canvas;
  let ctx;
  let wasmModule;
  let wasmModule3D;
  let animationId;
  let initPipes, updatePipes, getFramebuffer, cleanupPipes;
  let setFadeSpeed, setSpawnRate, setTurnProbability, setMaxPipes, setAnimationSpeed;
  let init3DPipes, update3DPipes, get3DFramebuffer, cleanup3DPipes;
  let set3DFadeSpeed, set3DSpawnRate, set3DTurnProbability, set3DMaxPipes;
  let handleMouseDown, handleMouseUp, handleMouseMove;
  let showSettings = true;
  let animationDelay = 1000 / 60; // Default 60 FPS
  let is3D = false; // Start in 2D mode until 3D is fixed
  let is3DAvailable = false;
  
  onMount(async () => {
    // Load 2D WASM module
    try {
      console.log('Loading 2D WASM module...');
      
      // Import ES6 module
      const createPipesModule = (await import('../wasm/pipes.js')).default;
      wasmModule = await createPipesModule();
      console.log('2D WASM module loaded');
      
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
      
      // Set canvas size
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
      
      // Try to load 3D module
      try {
        console.log('Loading 3D WASM module...');
        const createPipes3DModule = (await import('../wasm/pipes_3d.js')).default;
        wasmModule3D = await createPipes3DModule();
        console.log('3D WASM module loaded');
        
        // Get 3D exported functions
        init3DPipes = wasmModule3D.cwrap('init_3d_pipes', null, ['number', 'number']);
        update3DPipes = wasmModule3D.cwrap('update_3d_pipes', null, []);
        get3DFramebuffer = wasmModule3D.cwrap('get_3d_framebuffer', 'number', []);
        cleanup3DPipes = wasmModule3D.cwrap('cleanup_3d_pipes', null, []);
        
        // Get 3D parameter setters
        set3DFadeSpeed = wasmModule3D.cwrap('set_3d_fade_speed', null, ['number']);
        set3DSpawnRate = wasmModule3D.cwrap('set_3d_spawn_rate', null, ['number']);
        set3DTurnProbability = wasmModule3D.cwrap('set_3d_turn_probability', null, ['number']);
        set3DMaxPipes = wasmModule3D.cwrap('set_3d_max_pipes', null, ['number']);
        
        // Mouse handlers
        handleMouseDown = wasmModule3D.cwrap('handle_mouse_down', null, ['number', 'number']);
        handleMouseUp = wasmModule3D.cwrap('handle_mouse_up', null, []);
        handleMouseMove = wasmModule3D.cwrap('handle_mouse_move', null, ['number', 'number']);
        
        is3DAvailable = true;
        
        // Initialize 3D mode if starting in 3D
        if (is3D) {
          init3DPipes(canvas.width, canvas.height);
        }
      } catch (error) {
        console.warn('3D module not available:', error);
        is3DAvailable = false;
        is3D = false; // Fall back to 2D if 3D fails
      }
      
      // Initialize 2D mode if not in 3D
      if (!is3D) {
        initPipes(canvas.width, canvas.height);
      }
      
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
    if (is3D && wasmModule3D) {
      try {
        // Update 3D pipes (Raylib handles its own rendering)
        update3DPipes();
      } catch (error) {
        console.error('3D update error:', error);
        // Fall back to 2D
        is3D = false;
        toggle3D();
        return;
      }
    } else if (ctx && wasmModule) {
      // Update 2D pipes
      updatePipes();
      
      // Get 2D framebuffer
      const bufferPtr = getFramebuffer();
      
      if (bufferPtr && wasmModule.HEAPU8) {
        const dataLength = canvas.width * canvas.height * 4;
        const buffer = new Uint8ClampedArray(wasmModule.HEAPU8.buffer, bufferPtr, dataLength);
        
        const imageData = new ImageData(buffer, canvas.width, canvas.height);
        ctx.putImageData(imageData, 0, 0);
      }
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
    if (is3D && init3DPipes) {
      init3DPipes(window.innerWidth, window.innerHeight);
    } else if (canvas && initPipes) {
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
      initPipes(canvas.width, canvas.height);
    }
  }
  
  function toggle3D() {
    is3D = !is3D;
    
    if (is3D && init3DPipes) {
      try {
        // Hide our canvas and show Raylib's
        canvas.style.display = 'none';
        init3DPipes(window.innerWidth, window.innerHeight);
        
        // Give Raylib time to create its canvas
        setTimeout(() => {
          // Find and show Raylib's canvas
          const raylibCanvas = document.querySelector('canvas:not(#pipes-canvas)');
          if (raylibCanvas) {
            raylibCanvas.style.position = 'absolute';
            raylibCanvas.style.top = '0';
            raylibCanvas.style.left = '0';
            raylibCanvas.style.width = '100%';
            raylibCanvas.style.height = '100%';
            
            // Set up mouse events for Raylib canvas
            setupRaylibEvents();
          }
        }, 100);
      } catch (error) {
        console.error('Failed to initialize 3D mode:', error);
        // Fall back to 2D
        is3D = false;
        canvas.style.display = 'block';
        if (initPipes) {
          initPipes(canvas.width, canvas.height);
        }
      }
    } else {
      // Show our canvas and hide Raylib's
      canvas.style.display = 'block';
      
      const raylibCanvas = document.querySelector('canvas:not(#pipes-canvas)');
      if (raylibCanvas) {
        raylibCanvas.style.display = 'none';
      }
      
      if (initPipes) {
        initPipes(canvas.width, canvas.height);
      }
    }
  }
  
  function handleCanvasMouseDown(e) {
    if (is3D && handleMouseDown) {
      const target = e.target;
      const rect = target.getBoundingClientRect();
      handleMouseDown(e.clientX - rect.left, e.clientY - rect.top);
    }
  }
  
  function handleCanvasMouseUp(e) {
    if (is3D && handleMouseUp) {
      handleMouseUp();
    }
  }
  
  function handleCanvasMouseMove(e) {
    if (is3D && handleMouseMove) {
      const target = e.target;
      const rect = target.getBoundingClientRect();
      handleMouseMove(e.clientX - rect.left, e.clientY - rect.top);
    }
  }
  
  // Set up mouse event listeners for Raylib canvas when switching to 3D
  function setupRaylibEvents() {
    const raylibCanvas = document.querySelector('canvas:not(#pipes-canvas)');
    if (raylibCanvas) {
      raylibCanvas.addEventListener('mousedown', handleCanvasMouseDown);
      raylibCanvas.addEventListener('mouseup', handleCanvasMouseUp);
      raylibCanvas.addEventListener('mousemove', handleCanvasMouseMove);
    }
  }
</script>

<svelte:window on:resize={handleResize} />

<canvas 
  id="pipes-canvas"
  on:mousedown={handleCanvasMouseDown}
  on:mouseup={handleCanvasMouseUp}
  on:mousemove={handleCanvasMouseMove}
/>

<!-- Container for Raylib canvas -->
<div id="raylib-container" style="display: none; position: absolute; top: 0; left: 0; width: 100%; height: 100%;"></div>

{#if showSettings && setFadeSpeed}
  <Settings 
    setFadeSpeed={is3D ? set3DFadeSpeed : setFadeSpeed}
    setSpawnRate={is3D ? set3DSpawnRate : setSpawnRate}
    setTurnProbability={is3D ? set3DTurnProbability : setTurnProbability}
    setMaxPipes={is3D ? set3DMaxPipes : setMaxPipes}
    setAnimationSpeed={updateAnimationSpeed}
  />
{/if}

<div class="controls">
  {#if is3DAvailable}
    <button 
      class="mode-toggle"
      on:click={toggle3D}
    >
      {is3D ? '3D' : '2D'} Mode
    </button>
  {/if}
  
  <button 
    class="settings-toggle" 
    on:click={() => showSettings = !showSettings}
  >
    {showSettings ? 'Hide' : 'Show'} Settings
  </button>
</div>

{#if is3D}
  <div class="hint">Click and drag to rotate camera</div>
{/if}

<style>
  #pipes-canvas {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    cursor: default;
  }
  
  #pipes-canvas:active {
    cursor: move;
  }
  
  .controls {
    position: absolute;
    top: 20px;
    right: 20px;
    display: flex;
    gap: 10px;
    z-index: 100;
  }
  
  .settings-toggle, .mode-toggle {
    background: rgba(0, 0, 0, 0.8);
    color: white;
    border: 1px solid rgba(255, 255, 255, 0.2);
    padding: 0.5rem 1rem;
    border-radius: 4px;
    cursor: pointer;
  }
  
  .settings-toggle:hover, .mode-toggle:hover {
    background: rgba(0, 0, 0, 0.9);
    border-color: rgba(255, 255, 255, 0.4);
  }
  
  .mode-toggle {
    background: rgba(255, 62, 0, 0.8);
  }
  
  .mode-toggle:hover {
    background: rgba(255, 62, 0, 0.9);
  }
  
  .hint {
    position: absolute;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%);
    background: rgba(0, 0, 0, 0.7);
    color: white;
    padding: 0.5rem 1rem;
    border-radius: 4px;
    font-size: 0.9rem;
    pointer-events: none;
  }
</style>