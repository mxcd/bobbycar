<template>
  <div>
    <div class="row justify-around">
      <div class="h4" v-if="errorMessage">{{ errorMessage }}</div>
      <h2>{{ message }}</h2>
      <div class="row">
        <div class="col">
          <q-slider
            v-model="left"
            :min="-20"
            :max="100"
            vertical
            label
            reverse
            label-always
            switch-label-side
          />
        </div>
        <div class="col">
          <q-slider
            v-model="right"
            :min="-20"
            :max="100"
            color="green"
            vertical
            reverse
            label-always
          />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, watchEffect } from "vue";
import { emitter } from "src/boot/mitt";

let socket: WebSocket;
let socketInit = false;

const message = ref("not connected");

const left = ref(0);
const right = ref(0);

const errorMessage = ref("");

const initSocket = () => {
  const s = new WebSocket(`ws://localhost:8090`);
  s.onerror = function (error) {
    console.log(`[error]`, error);
  };
  s.onopen = function (e) {
    console.log("[open] Connection established");
    socketInit = true;
  };
  return s;
};

interface Props {
  title: string;
}
const props = withDefaults(defineProps<Props>(), {
  title: () => "foo",
});

emitter.on("keypress", (args: any) => {
  let key = args as string;
  key = key.toLowerCase();
  console.log(`[keypress]`, key);
  if (key === "q") {
    message.value = "FASTER";
    left.value = Math.min(100, left.value + 5);
    right.value = Math.min(100, right.value + 5);
  } else if (key === "a") {
    message.value = "SLOWER";
    left.value = Math.max(-20, left.value - 5);
    right.value = Math.max(-20, right.value - 5);
  } /*else if (key === "w") {
    right.value = Math.min(100, right.value + 5);
  } else if (key === "s") {
    right.value = Math.max(-20, right.value - 5);
  } */ else if (key === "c") {
    message.value = "CONNECTED";
    socket = initSocket();
    socketInit = true;
  } else if (key === "x" || key === " ") {
    message.value = "CUTOFF";
    right.value = 0;
    left.value = 0;
    setTimeout(() => {
      socket.close();
      socketInit = false;
    }, 500);
  }
});

const sendTorqueRequest = () => {
  if (!socketInit) {
    errorMessage.value = "Socket not initialized. Press 'c' to connect.";
    return;
  } else {
    errorMessage.value = "";
  }
  const data = {
    left: left.value,
    right: right.value,
  };
  console.log(`[send]`, data);
  socket.send(JSON.stringify(data));
};

watch(
  () => left.value,
  () => {
    sendTorqueRequest();
  }
);
watch(
  () => right.value,
  () => {
    sendTorqueRequest();
  }
);
</script>
