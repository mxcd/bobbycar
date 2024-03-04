<template>
  <div></div>
</template>

<script setup lang="ts">
interface Props {
  title: string;
  active: boolean;
}
const props = withDefaults(defineProps<Props>(), {
  title: () => "foo",
});
</script>
