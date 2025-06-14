<script>
  import { onMount, onDestroy } from 'svelte';
  
  let canvas;
  let ctx;
  let wasmModule;
  let animationId;
  let initPipes, updatePipes, getFramebuffer, cleanupPipes;
  
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
    
    animationId = requestAnimationFrame(animate);
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

<style>
  /* Styles are in App.svelte */
</style>