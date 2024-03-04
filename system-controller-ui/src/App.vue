<template>
  <router-view />
</template>

<script setup lang="ts">
import { emitter } from "./boot/mitt";

document.addEventListener("keypress", function (event: any) {
  emitter.emit("keypress", event.key);
});
</script>
