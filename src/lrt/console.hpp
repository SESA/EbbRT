namespace ebbrt {
  namespace lrt {
    /** The early boot console to be used from within the lrt */
    namespace console {
      /** Initialize the console */
      void init();
      /**
       * Write a character to the console
       * @param[in] c The character to write
       */
      void write(char c);
      /**
       * Write a specified amount of data to the console
       * @param[in] str The data to write
       * @param[in] len The amount of data to write
       */
      int write(const char *str, int len);
      /**
       * Write a null terminated string to the console
       * @param[in] str The null terminated string
       */
      void write(const char *str);
    }
  }
}
