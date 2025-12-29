export async function GET() {
  return new Response("0.1.0", {
    status: 200,
    headers: {
      "Content-Type": "text/plain",
    },
  });
}
