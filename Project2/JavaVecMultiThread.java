import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.StringTokenizer;
import java.util.concurrent.*;

public class JavaVecMultiThread{
    static final int CYCLE_TIME = 100;
    private static int THREADS = 1;

    static class VectorAccumulator {
        private double doubleSum = 0;
        private int intSum = 0;

        public synchronized void add(double value) { doubleSum += value; }
        public synchronized void add(int value) { intSum += value; }
        public double getDoubleSum() { return doubleSum; }
        public int getIntSum() { return intSum; }
    }

    public static int calculateVectorInt(int[] data, int N, int dim) {
        VectorAccumulator acc = new VectorAccumulator();
        ExecutorService executor = Executors.newFixedThreadPool(THREADS);

        for (int i = 1; i <= N/2; i++) {
            final int currentI = i;
            executor.execute(() -> {
                int p1 = (2 * currentI - 2) * dim;
                int p2 = (2 * currentI - 1) * dim;
                int localSum = 0;

                for (int j = 0; j < dim; j++) {
                    localSum += data[p1 + j] * data[p2 + j];
                }

                acc.add(localSum);
            });
        }

        executor.shutdown();
        try {
            executor.awaitTermination(1, TimeUnit.MINUTES);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return acc.getIntSum();
    }

    public static int calculateVectorShort(short[] data, int N, int dim) {
        VectorAccumulator acc = new VectorAccumulator();
        ExecutorService executor = Executors.newFixedThreadPool(THREADS);

        for (int i = 1; i <= N/2; i++) {
            final int currentI = i;
            executor.execute(() -> {
                int p1 = (2 * currentI - 2) * dim;
                int p2 = (2 * currentI - 1) * dim;
                int localSum = 0;

                for (int j = 0; j < dim; j++) {
                    localSum += data[p1 + j] * data[p2 + j];
                }

                acc.add(localSum);
            });
        }

        executor.shutdown();
        try {
            executor.awaitTermination(1, TimeUnit.MINUTES);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return acc.getIntSum();
    }

    public static int calculateVectorByte(byte[] data, int N, int dim) {
        VectorAccumulator acc = new VectorAccumulator();
        ExecutorService executor = Executors.newFixedThreadPool(THREADS);

        for (int i = 1; i <= N/2; i++) {
            final int currentI = i;
            executor.execute(() -> {
                int p1 = (2 * currentI - 2) * dim;
                int p2 = (2 * currentI - 1) * dim;
                int localSum = 0;

                for (int j = 0; j < dim; j++) {
                    localSum += data[p1 + j] * data[p2 + j];
                }

                acc.add(localSum);
            });
        }

        executor.shutdown();
        try {
            executor.awaitTermination(1, TimeUnit.MINUTES);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return acc.getIntSum();
    }

    public static double calculateVectorDouble(double[] data, int N, int dim) {
        VectorAccumulator acc = new VectorAccumulator();
        ExecutorService executor = Executors.newFixedThreadPool(THREADS);

        for (int i = 1; i <= N/2; i++) {
            final int currentI = i;
            executor.execute(() -> {
                int p1 = (2 * currentI - 2) * dim;
                int p2 = (2 * currentI - 1) * dim;
                double localSum = 0;

                for (int j = 0; j < dim; j++) {
                    localSum += data[p1 + j] * data[p2 + j];
                }

                acc.add(localSum);
            });
        }

        executor.shutdown();
        try {
            executor.awaitTermination(1, TimeUnit.MINUTES);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return acc.getDoubleSum();
    }

    public static float calculateVectorFloat(float[] data, int N, int dim) {
        VectorAccumulator acc = new VectorAccumulator();
        ExecutorService executor = Executors.newFixedThreadPool(THREADS);

        for (int i = 1; i <= N / 2; i++) {
            final int currentI = i;
            executor.execute(() -> {
                int p1 = (2 * currentI - 2) * dim;
                int p2 = (2 * currentI - 1) * dim;
                float localSum = 0;

                for (int j = 0; j < dim; j++) {
                    localSum += data[p1 + j] * data[p2 + j];
                }

                acc.add((double) localSum);
            });
        }

        executor.shutdown();
        try {
            executor.awaitTermination(1, TimeUnit.MINUTES);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return (float)acc.getDoubleSum();
    }

    public static void main(String[] args) throws IOException {
        String inputFile = "randomCase.txt";
        QReader sc = new QReader(new File(inputFile));
        int type = sc.nextInt();
        int dim = sc.nextInt();
        int N = sc.nextInt();

        double sum = 0;
        long totalTime = 0;

        //warm up get time method
        for (int i = 0; i < 10000; i++) System.nanoTime();

        switch (type) {
            case 1 -> { // int
                int[] intData = new int[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        intData[i * dim + j] = sc.nextInt();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorInt(intData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 2 -> { // short
                short[] shortData = new short[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        shortData[i * dim + j] = sc.nextShort();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorShort(shortData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 3 -> { // byte
                byte[] byteData = new byte[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        byteData[i * dim + j] = sc.nextByte();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorByte(byteData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 4 -> { // double
                double[] doubleData = new double[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        doubleData[i * dim + j] = sc.nextDouble();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorDouble(doubleData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 5 -> { // float
                float[] floatData = new float[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        floatData[i * dim + j] = sc.nextFloat();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorFloat(floatData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            default -> throw new IllegalArgumentException("Invalid type: " + type);
        }

        long avgTime = totalTime / CYCLE_TIME;
        System.out.printf("sum = %f\n", sum);
        System.out.println("Calculations finished in avg "+avgTime+" nano second");
        System.out.println("Calculations finished in avg "+avgTime/1e6+" millisecond");
    }
}
/**
 * FOR TEST USE
 * case 5 -> { // float
     float[] floatData = new float[N * dim];
     for (int i = 0; i < N; i++) {
         for (int j = 0; j < dim; j++) {
             floatData[i * dim + j] = sc.nextFloat();
         }
     }
     THREADS = 1;
     int max_thread = 32;
     float base_time = 0;
     while(THREADS<max_thread) {
         totalTime = 0;
         for (int i = 0; i < CYCLE_TIME; i++) {
             long start = System.nanoTime();
             sum = calculateVectorFloat(floatData, N, dim);
             totalTime += System.nanoTime() - start;
         }
         if(THREADS==1){
             base_time = totalTime;
         }
         System.out.println("Thread num = "+THREADS+", time(ns) = "+totalTime+", ratio = "+(float)totalTime/base_time);
         THREADS++;
     }

 }
 * */